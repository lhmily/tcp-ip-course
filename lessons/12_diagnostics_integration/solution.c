#include "lesson.h"

#include <stdio.h>
#include <string.h>

#define TCPIP_L12_ETHERNET_HEADER 14U
#define TCPIP_L12_IPV4_MIN_HEADER 20U
#define TCPIP_L12_ARP_FIXED_HEADER 8U
#define TCPIP_L12_UDP_HEADER 8U
#define TCPIP_L12_DNS_HEADER 12U
#define TCPIP_L12_ICMP_HEADER 8U
#define TCPIP_L12_TCP_MIN_HEADER 20U

static uint16_t tcpip_l12_read_u16(const uint8_t *data) {
  return (uint16_t)(((uint16_t)data[0] << 8U) | (uint16_t)data[1]);
}

static uint32_t tcpip_l12_read_u32(const uint8_t *data) {
  return ((uint32_t)data[0] << 24U) | ((uint32_t)data[1] << 16U) |
         ((uint32_t)data[2] << 8U) | (uint32_t)data[3];
}

static void tcpip_l12_add_layer(tcpip_l12_report *report, tcpip_l12_layer layer) {
  if (report->layer_count < TCPIP_L12_MAX_LAYERS) {
    report->layers[report->layer_count] = layer;
    report->layer_count += 1U;
  }
}

static tcpip_l12_status tcpip_l12_truncated(tcpip_l12_report *report, size_t parsed) {
  report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_TRUNCATED;
  report->parsed_length = parsed;
  return TCPIP_L12_TRUNCATED;
}

static tcpip_l12_status tcpip_l12_malformed(tcpip_l12_report *report, size_t parsed) {
  report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_MALFORMED;
  report->parsed_length = parsed;
  return TCPIP_L12_MALFORMED;
}

static uint32_t tcpip_l12_checksum_add(uint32_t sum, const uint8_t *data, size_t len) {
  size_t offset = 0U;

  while (len - offset >= 2U) {
    sum += (uint32_t)tcpip_l12_read_u16(data + offset);
    offset += 2U;
  }
  if (offset < len) {
    sum += (uint32_t)data[offset] << 8U;
  }
  return sum;
}

static uint16_t tcpip_l12_checksum_finish(uint32_t sum) {
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)sum;
}

static int tcpip_l12_checksum_valid(const uint8_t *data, size_t len) {
  return tcpip_l12_checksum_finish(tcpip_l12_checksum_add(0U, data, len)) == UINT16_C(0xffff);
}

static int tcpip_l12_transport_checksum_valid(
    const uint8_t *ipv4, uint8_t protocol, const uint8_t *segment, size_t segment_len) {
  uint32_t sum = 0U;

  sum = tcpip_l12_checksum_add(sum, ipv4 + 12U, 8U);
  sum += (uint32_t)protocol;
  sum += (uint32_t)segment_len;
  sum = tcpip_l12_checksum_add(sum, segment, segment_len);
  return tcpip_l12_checksum_finish(sum) == UINT16_C(0xffff);
}

static void tcpip_l12_record_checksum(
    tcpip_l12_report *report, int valid, tcpip_l12_diagnostic targeted_diagnostic) {
  report->checksums_checked += 1U;
  if (valid != 0) {
    report->checksums_valid += 1U;
  } else {
    report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_CHECKSUM_MISMATCH;
    report->diagnostics |= (uint32_t)targeted_diagnostic;
  }
}

static int tcpip_l12_http_prefix(const uint8_t *data, size_t len) {
  const char *const prefixes[] = {
      "GET ", "HEAD ", "POST ", "PUT ", "DELETE ", "OPTIONS ", "HTTP/"};
  size_t index;

  for (index = 0U; index < sizeof(prefixes) / sizeof(prefixes[0]); index += 1U) {
    size_t prefix_len = strlen(prefixes[index]);
    if (len >= prefix_len && memcmp(data, prefixes[index], prefix_len) == 0) {
      return 1;
    }
  }
  return 0;
}

static int tcpip_l12_http_headers_complete(const uint8_t *data, size_t len) {
  size_t index;

  if (len < 4U) {
    return 0;
  }
  for (index = 0U; index + 4U <= len; index += 1U) {
    if (data[index] == (uint8_t)'\r' && data[index + 1U] == (uint8_t)'\n' &&
        data[index + 2U] == (uint8_t)'\r' && data[index + 3U] == (uint8_t)'\n') {
      return 1;
    }
  }
  return 0;
}

static tcpip_l12_status tcpip_l12_parse_dns(
    const uint8_t *dns, size_t dns_len, size_t frame_offset, tcpip_l12_report *report) {
  size_t cursor;
  uint16_t question;

  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_DNS);
  if (dns_len < TCPIP_L12_DNS_HEADER) {
    return tcpip_l12_truncated(report, frame_offset + dns_len);
  }

  report->dns_identifier = tcpip_l12_read_u16(dns);
  report->dns_question_count = tcpip_l12_read_u16(dns + 4U);
  cursor = TCPIP_L12_DNS_HEADER;

  for (question = 0U; question < report->dns_question_count; question += 1U) {
    int name_done = 0;
    size_t label_count = 0U;

    while (name_done == 0) {
      uint8_t label_len;
      if (cursor >= dns_len) {
        return tcpip_l12_truncated(report, frame_offset + cursor);
      }
      label_len = dns[cursor];
      cursor += 1U;
      if (label_len == 0U) {
        name_done = 1;
      } else if ((label_len & UINT8_C(0xc0)) == UINT8_C(0xc0)) {
        uint16_t pointer;
        if (cursor >= dns_len) {
          return tcpip_l12_truncated(report, frame_offset + cursor);
        }
        pointer = (uint16_t)((((uint16_t)label_len & UINT16_C(0x003f)) << 8U) |
                             (uint16_t)dns[cursor]);
        cursor += 1U;
        if ((size_t)pointer >= dns_len) {
          return tcpip_l12_malformed(report, frame_offset + cursor);
        }
        name_done = 1;
      } else {
        if ((label_len & UINT8_C(0xc0)) != 0U || label_len > 63U) {
          return tcpip_l12_malformed(report, frame_offset + cursor);
        }
        if ((size_t)label_len > dns_len - cursor) {
          return tcpip_l12_truncated(report, frame_offset + cursor);
        }
        cursor += (size_t)label_len;
      }
      label_count += 1U;
      if (label_count > 128U) {
        return tcpip_l12_malformed(report, frame_offset + cursor);
      }
    }

    if (dns_len - cursor < 4U) {
      return tcpip_l12_truncated(report, frame_offset + cursor);
    }
    cursor += 4U;
  }

  report->parsed_length = frame_offset + dns_len;
  return TCPIP_L12_OK;
}

static tcpip_l12_status tcpip_l12_parse_udp(
    const uint8_t *ipv4,
    const uint8_t *udp,
    size_t ip_payload_len,
    size_t frame_offset,
    tcpip_l12_report *report) {
  uint16_t udp_len;
  uint16_t udp_checksum;

  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_UDP);
  if (ip_payload_len < TCPIP_L12_UDP_HEADER) {
    return tcpip_l12_truncated(report, frame_offset + ip_payload_len);
  }

  report->source_port = tcpip_l12_read_u16(udp);
  report->destination_port = tcpip_l12_read_u16(udp + 2U);
  udp_len = tcpip_l12_read_u16(udp + 4U);
  udp_checksum = tcpip_l12_read_u16(udp + 6U);
  if (udp_len < TCPIP_L12_UDP_HEADER) {
    return tcpip_l12_malformed(report, frame_offset + TCPIP_L12_UDP_HEADER);
  }
  if ((size_t)udp_len > ip_payload_len) {
    return tcpip_l12_truncated(report, frame_offset + ip_payload_len);
  }

  report->payload_length = (size_t)udp_len - TCPIP_L12_UDP_HEADER;
  if (udp_checksum != 0U) {
    tcpip_l12_record_checksum(
        report,
        tcpip_l12_transport_checksum_valid(ipv4, UINT8_C(17), udp, (size_t)udp_len),
        TCPIP_L12_DIAG_TRANSPORT_CHECKSUM);
  }

  if (report->source_port == 53U || report->destination_port == 53U) {
    return tcpip_l12_parse_dns(
        udp + TCPIP_L12_UDP_HEADER,
        report->payload_length,
        frame_offset + TCPIP_L12_UDP_HEADER,
        report);
  }

  report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_UNSUPPORTED;
  report->parsed_length = frame_offset + (size_t)udp_len;
  return TCPIP_L12_OK;
}

static tcpip_l12_status tcpip_l12_parse_tcp(
    const uint8_t *ipv4,
    const uint8_t *tcp,
    size_t ip_payload_len,
    size_t frame_offset,
    tcpip_l12_report *report) {
  size_t tcp_header_len;
  const uint8_t *payload;

  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_TCP);
  if (ip_payload_len < TCPIP_L12_TCP_MIN_HEADER) {
    return tcpip_l12_truncated(report, frame_offset + ip_payload_len);
  }

  report->source_port = tcpip_l12_read_u16(tcp);
  report->destination_port = tcpip_l12_read_u16(tcp + 2U);
  tcp_header_len = (size_t)(tcp[12] >> 4U) * 4U;
  report->tcp_flags = tcp[13];
  if (tcp_header_len < TCPIP_L12_TCP_MIN_HEADER) {
    return tcpip_l12_malformed(report, frame_offset + TCPIP_L12_TCP_MIN_HEADER);
  }
  if (tcp_header_len > ip_payload_len) {
    return tcpip_l12_truncated(report, frame_offset + ip_payload_len);
  }

  tcpip_l12_record_checksum(
      report,
      tcpip_l12_transport_checksum_valid(ipv4, UINT8_C(6), tcp, ip_payload_len),
      TCPIP_L12_DIAG_TRANSPORT_CHECKSUM);
  report->payload_length = ip_payload_len - tcp_header_len;
  payload = tcp + tcp_header_len;

  if (report->source_port == 80U || report->destination_port == 80U ||
      report->source_port == 8080U || report->destination_port == 8080U) {
    tcpip_l12_add_layer(report, TCPIP_L12_LAYER_HTTP);
    report->http_message_length = report->payload_length;
    if (tcpip_l12_http_prefix(payload, report->payload_length) == 0) {
      return tcpip_l12_malformed(report, frame_offset + tcp_header_len);
    }
    if (tcpip_l12_http_headers_complete(payload, report->payload_length) == 0) {
      return tcpip_l12_truncated(report, frame_offset + ip_payload_len);
    }
    report->parsed_length = frame_offset + ip_payload_len;
    return TCPIP_L12_OK;
  }

  report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_UNSUPPORTED;
  report->parsed_length = frame_offset + ip_payload_len;
  return TCPIP_L12_OK;
}

static tcpip_l12_status tcpip_l12_parse_icmp(
    const uint8_t *icmp,
    size_t ip_payload_len,
    size_t frame_offset,
    tcpip_l12_report *report) {
  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_ICMP);
  if (ip_payload_len < TCPIP_L12_ICMP_HEADER) {
    return tcpip_l12_truncated(report, frame_offset + ip_payload_len);
  }

  report->icmp_type = icmp[0];
  report->payload_length = ip_payload_len - TCPIP_L12_ICMP_HEADER;
  tcpip_l12_record_checksum(
      report, tcpip_l12_checksum_valid(icmp, ip_payload_len), TCPIP_L12_DIAG_TRANSPORT_CHECKSUM);
  report->parsed_length = frame_offset + ip_payload_len;
  return TCPIP_L12_OK;
}

static tcpip_l12_status tcpip_l12_parse_ipv4(
    const uint8_t *ipv4, size_t available, size_t frame_offset, tcpip_l12_report *report) {
  size_t header_len;
  size_t total_len;
  size_t payload_len;
  uint16_t fragment;

  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_IPV4);
  if (available < TCPIP_L12_IPV4_MIN_HEADER) {
    return tcpip_l12_truncated(report, frame_offset + available);
  }
  if ((ipv4[0] >> 4U) != 4U) {
    return tcpip_l12_malformed(report, frame_offset + 1U);
  }

  header_len = (size_t)(ipv4[0] & UINT8_C(0x0f)) * 4U;
  if (header_len < TCPIP_L12_IPV4_MIN_HEADER) {
    return tcpip_l12_malformed(report, frame_offset + 1U);
  }
  if (header_len > available) {
    return tcpip_l12_truncated(report, frame_offset + available);
  }

  total_len = (size_t)tcpip_l12_read_u16(ipv4 + 2U);
  if (total_len < header_len) {
    return tcpip_l12_malformed(report, frame_offset + header_len);
  }
  if (total_len > available) {
    return tcpip_l12_truncated(report, frame_offset + available);
  }

  report->ip_protocol = ipv4[9];
  report->source_ipv4 = tcpip_l12_read_u32(ipv4 + 12U);
  report->destination_ipv4 = tcpip_l12_read_u32(ipv4 + 16U);
  tcpip_l12_record_checksum(
      report, tcpip_l12_checksum_valid(ipv4, header_len), TCPIP_L12_DIAG_IPV4_CHECKSUM);

  fragment = tcpip_l12_read_u16(ipv4 + 6U);
  if ((fragment & UINT16_C(0x3fff)) != 0U) {
    report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_UNSUPPORTED;
    report->payload_length = total_len - header_len;
    report->parsed_length = frame_offset + total_len;
    return TCPIP_L12_OK;
  }

  payload_len = total_len - header_len;
  if (report->ip_protocol == 1U) {
    return tcpip_l12_parse_icmp(
        ipv4 + header_len, payload_len, frame_offset + header_len, report);
  }
  if (report->ip_protocol == 17U) {
    return tcpip_l12_parse_udp(
        ipv4, ipv4 + header_len, payload_len, frame_offset + header_len, report);
  }
  if (report->ip_protocol == 6U) {
    return tcpip_l12_parse_tcp(
        ipv4, ipv4 + header_len, payload_len, frame_offset + header_len, report);
  }

  report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_UNSUPPORTED;
  report->payload_length = payload_len;
  report->parsed_length = frame_offset + total_len;
  return TCPIP_L12_OK;
}

static tcpip_l12_status tcpip_l12_parse_arp(
    const uint8_t *arp, size_t available, size_t frame_offset, tcpip_l12_report *report) {
  size_t address_bytes;
  size_t arp_len;

  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_ARP);
  if (available < TCPIP_L12_ARP_FIXED_HEADER) {
    return tcpip_l12_truncated(report, frame_offset + available);
  }

  report->arp_opcode = tcpip_l12_read_u16(arp + 6U);
  address_bytes = (size_t)arp[4] + (size_t)arp[5];
  if (address_bytes > (SIZE_MAX - TCPIP_L12_ARP_FIXED_HEADER) / 2U) {
    return tcpip_l12_malformed(report, frame_offset + TCPIP_L12_ARP_FIXED_HEADER);
  }
  arp_len = TCPIP_L12_ARP_FIXED_HEADER + (2U * address_bytes);
  if (arp_len > available) {
    return tcpip_l12_truncated(report, frame_offset + available);
  }
  if (tcpip_l12_read_u16(arp) != 1U || tcpip_l12_read_u16(arp + 2U) != UINT16_C(0x0800) ||
      arp[4] != 6U || arp[5] != 4U ||
      (report->arp_opcode != 1U && report->arp_opcode != 2U)) {
    return tcpip_l12_malformed(report, frame_offset + arp_len);
  }

  report->payload_length = arp_len;
  report->parsed_length = frame_offset + arp_len;
  return TCPIP_L12_OK;
}

tcpip_l12_status tcpip_l12_diagnose_frame(
    const uint8_t *data, size_t len, tcpip_l12_report *report) {
  if (report == NULL || (data == NULL && len != 0U)) {
    return TCPIP_L12_INVALID_ARGUMENT;
  }

  memset(report, 0, sizeof(*report));
  report->frame_length = len;
  tcpip_l12_add_layer(report, TCPIP_L12_LAYER_ETHERNET);
  if (len < TCPIP_L12_ETHERNET_HEADER) {
    return tcpip_l12_truncated(report, len);
  }

  report->ether_type = tcpip_l12_read_u16(data + 12U);
  report->parsed_length = TCPIP_L12_ETHERNET_HEADER;
  if (report->ether_type == UINT16_C(0x0806)) {
    return tcpip_l12_parse_arp(
        data + TCPIP_L12_ETHERNET_HEADER,
        len - TCPIP_L12_ETHERNET_HEADER,
        TCPIP_L12_ETHERNET_HEADER,
        report);
  }
  if (report->ether_type == UINT16_C(0x0800)) {
    return tcpip_l12_parse_ipv4(
        data + TCPIP_L12_ETHERNET_HEADER,
        len - TCPIP_L12_ETHERNET_HEADER,
        TCPIP_L12_ETHERNET_HEADER,
        report);
  }

  report->diagnostics |= (uint32_t)TCPIP_L12_DIAG_UNSUPPORTED;
  return TCPIP_L12_OK;
}

static const char *tcpip_l12_layer_name(tcpip_l12_layer layer) {
  switch (layer) {
    case TCPIP_L12_LAYER_ETHERNET:
      return "ethernet";
    case TCPIP_L12_LAYER_ARP:
      return "arp";
    case TCPIP_L12_LAYER_IPV4:
      return "ipv4";
    case TCPIP_L12_LAYER_ICMP:
      return "icmp";
    case TCPIP_L12_LAYER_UDP:
      return "udp";
    case TCPIP_L12_LAYER_DNS:
      return "dns";
    case TCPIP_L12_LAYER_TCP:
      return "tcp";
    case TCPIP_L12_LAYER_HTTP:
      return "http";
    default:
      return NULL;
  }
}

static int tcpip_l12_append_text(char *buffer, size_t cap, size_t *used, const char *text) {
  size_t text_len = strlen(text);

  if (text_len > cap - *used) {
    return 0;
  }
  memcpy(buffer + *used, text, text_len);
  *used += text_len;
  return 1;
}

tcpip_l12_status tcpip_l12_format_report(
    const tcpip_l12_report *report, char *out, size_t cap, size_t *written) {
  char layers[64];
  char summary[512];
  size_t layer_used = 0U;
  size_t index;
  int count;
  size_t required;
  size_t copied;

  if (report == NULL || written == NULL || (out == NULL && cap != 0U) ||
      report->layer_count > TCPIP_L12_MAX_LAYERS) {
    return TCPIP_L12_INVALID_ARGUMENT;
  }

  for (index = 0U; index < report->layer_count; index += 1U) {
    const char *name = tcpip_l12_layer_name(report->layers[index]);
    if (name == NULL) {
      return TCPIP_L12_INVALID_ARGUMENT;
    }
    if (index != 0U && tcpip_l12_append_text(layers, sizeof(layers), &layer_used, ">") == 0) {
      return TCPIP_L12_INVALID_ARGUMENT;
    }
    if (tcpip_l12_append_text(layers, sizeof(layers), &layer_used, name) == 0) {
      return TCPIP_L12_INVALID_ARGUMENT;
    }
  }
  layers[layer_used] = '\0';

  count = snprintf(
      summary,
      sizeof(summary),
      "layers=%s diagnostics=0x%08lx frame=%zu parsed=%zu payload=%zu "
      "ethertype=0x%04x arp_opcode=%u ip_protocol=%u "
      "src=%u.%u.%u.%u dst=%u.%u.%u.%u ports=%u->%u "
      "icmp_type=%u dns_id=0x%04x dns_questions=%u tcp_flags=0x%02x "
      "http_bytes=%zu checksums=%u/%u",
      layers,
      (unsigned long)report->diagnostics,
      report->frame_length,
      report->parsed_length,
      report->payload_length,
      (unsigned)report->ether_type,
      (unsigned)report->arp_opcode,
      (unsigned)report->ip_protocol,
      (unsigned)((report->source_ipv4 >> 24U) & UINT32_C(0xff)),
      (unsigned)((report->source_ipv4 >> 16U) & UINT32_C(0xff)),
      (unsigned)((report->source_ipv4 >> 8U) & UINT32_C(0xff)),
      (unsigned)(report->source_ipv4 & UINT32_C(0xff)),
      (unsigned)((report->destination_ipv4 >> 24U) & UINT32_C(0xff)),
      (unsigned)((report->destination_ipv4 >> 16U) & UINT32_C(0xff)),
      (unsigned)((report->destination_ipv4 >> 8U) & UINT32_C(0xff)),
      (unsigned)(report->destination_ipv4 & UINT32_C(0xff)),
      (unsigned)report->source_port,
      (unsigned)report->destination_port,
      (unsigned)report->icmp_type,
      (unsigned)report->dns_identifier,
      (unsigned)report->dns_question_count,
      (unsigned)report->tcp_flags,
      report->http_message_length,
      report->checksums_valid,
      report->checksums_checked);
  if (count < 0 || (size_t)count >= sizeof(summary)) {
    return TCPIP_L12_CAPACITY;
  }

  required = (size_t)count;
  *written = required;
  if (cap != 0U) {
    copied = required;
    if (copied >= cap) {
      copied = cap - 1U;
    }
    memcpy(out, summary, copied);
    out[copied] = '\0';
  }
  if (cap <= required) {
    return TCPIP_L12_CAPACITY;
  }
  return TCPIP_L12_OK;
}
