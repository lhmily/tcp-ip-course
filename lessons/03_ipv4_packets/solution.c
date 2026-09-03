#include "lesson.h"

#include <string.h>

static uint16_t tcpip_l03_read_be16(const uint8_t *data, size_t offset) {
  return (uint16_t)(((uint16_t)data[offset] << 8U) | (uint16_t)data[offset + 1U]);
}

static void tcpip_l03_write_be16(uint8_t *data, size_t offset, uint16_t value) {
  data[offset] = (uint8_t)(value >> 8U);
  data[offset + 1U] = (uint8_t)(value & UINT16_C(0x00ff));
}

static uint16_t tcpip_l03_checksum(const uint8_t *data, size_t data_length) {
  uint32_t sum = 0U;
  size_t offset = 0U;

  while (data_length - offset >= 2U) {
    sum += (uint32_t)tcpip_l03_read_be16(data, offset);
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
    offset += 2U;
  }
  if (offset < data_length) {
    sum += (uint32_t)data[offset] << 8U;
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)~sum;
}

tcpip_l03_status tcpip_l03_parse_ipv4(
    const uint8_t *packet,
    size_t packet_length,
    tcpip_l03_ipv4_packet *out_packet) {
  tcpip_l03_ipv4_packet parsed;
  size_t header_length;
  uint16_t total_length;

  if (out_packet != NULL) {
    memset(out_packet, 0, sizeof(*out_packet));
  }
  if (packet == NULL || out_packet == NULL) {
    return TCPIP_L03_INVALID_ARGUMENT;
  }
  if (packet_length < TCPIP_L03_MIN_HEADER_LENGTH) {
    return TCPIP_L03_TRUNCATED;
  }

  memset(&parsed, 0, sizeof(parsed));
  parsed.version = (uint8_t)(packet[0U] >> 4U);
  parsed.ihl = (uint8_t)(packet[0U] & UINT8_C(0x0f));
  if (parsed.version != UINT8_C(4) || parsed.ihl < UINT8_C(5)) {
    return TCPIP_L03_MALFORMED;
  }

  header_length = (size_t)parsed.ihl * 4U;
  if (header_length > TCPIP_L03_MAX_HEADER_LENGTH) {
    return TCPIP_L03_MALFORMED;
  }
  if (packet_length < header_length) {
    return TCPIP_L03_TRUNCATED;
  }

  total_length = tcpip_l03_read_be16(packet, 2U);
  if ((tcpip_l03_read_be16(packet, 6U) & UINT16_C(0x8000)) != 0U) {
    return TCPIP_L03_MALFORMED;
  }
  if ((size_t)total_length < header_length) {
    return TCPIP_L03_MALFORMED;
  }
  if (packet_length < (size_t)total_length) {
    return TCPIP_L03_TRUNCATED;
  }
  if (tcpip_l03_checksum(packet, header_length) != 0U) {
    return TCPIP_L03_MALFORMED;
  }

  parsed.dscp_ecn = packet[1U];
  parsed.total_length = total_length;
  parsed.identification = tcpip_l03_read_be16(packet, 4U);
  parsed.flags_fragment = tcpip_l03_read_be16(packet, 6U);
  parsed.ttl = packet[8U];
  parsed.protocol = packet[9U];
  parsed.header_checksum = tcpip_l03_read_be16(packet, 10U);
  memcpy(parsed.source, packet + 12U, sizeof(parsed.source));
  memcpy(parsed.destination, packet + 16U, sizeof(parsed.destination));
  parsed.options_offset = TCPIP_L03_MIN_HEADER_LENGTH;
  parsed.options_length = header_length - TCPIP_L03_MIN_HEADER_LENGTH;
  parsed.payload_offset = header_length;
  parsed.payload_length = (size_t)total_length - header_length;
  *out_packet = parsed;
  return TCPIP_L03_OK;
}

tcpip_l03_status tcpip_l03_build_header(
    const tcpip_l03_ipv4_header_fields *fields,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_header_length) {
  uint8_t header[TCPIP_L03_MAX_HEADER_LENGTH] = {0U};
  size_t header_length;
  uint16_t checksum;

  if (out_header_length != NULL) {
    *out_header_length = 0U;
  }
  if (fields == NULL || destination == NULL || out_header_length == NULL) {
    return TCPIP_L03_INVALID_ARGUMENT;
  }
  if ((fields->options == NULL && fields->options_length != 0U) ||
      fields->options_length > TCPIP_L03_MAX_OPTIONS_LENGTH ||
      (fields->options_length % 4U) != 0U ||
      (fields->flags_fragment & UINT16_C(0x8000)) != 0U) {
    return TCPIP_L03_MALFORMED;
  }

  header_length = TCPIP_L03_MIN_HEADER_LENGTH + fields->options_length;
  if (destination_capacity < header_length) {
    return TCPIP_L03_CAPACITY;
  }

  header[0U] = (uint8_t)((UINT8_C(4) << 4U) | (uint8_t)(header_length / 4U));
  header[1U] = fields->dscp_ecn;
  tcpip_l03_write_be16(header, 2U, (uint16_t)header_length);
  tcpip_l03_write_be16(header, 4U, fields->identification);
  tcpip_l03_write_be16(header, 6U, fields->flags_fragment);
  header[8U] = fields->ttl;
  header[9U] = fields->protocol;
  memcpy(header + 12U, fields->source, sizeof(fields->source));
  memcpy(header + 16U, fields->destination, sizeof(fields->destination));
  if (fields->options_length != 0U) {
    memcpy(header + TCPIP_L03_MIN_HEADER_LENGTH, fields->options, fields->options_length);
  }

  checksum = tcpip_l03_checksum(header, header_length);
  tcpip_l03_write_be16(header, 10U, checksum);
  memcpy(destination, header, header_length);
  *out_header_length = header_length;
  return TCPIP_L03_OK;
}

tcpip_l03_status tcpip_l03_decrement_ttl(
    uint8_t *packet,
    size_t packet_length) {
  tcpip_l03_ipv4_packet parsed;
  tcpip_l03_status status;
  size_t header_length;
  uint16_t checksum;

  if (packet == NULL) {
    return TCPIP_L03_INVALID_ARGUMENT;
  }
  status = tcpip_l03_parse_ipv4(packet, packet_length, &parsed);
  if (status != TCPIP_L03_OK) {
    return status;
  }
  if (parsed.ttl <= UINT8_C(1)) {
    return TCPIP_L03_MALFORMED;
  }

  header_length = (size_t)parsed.ihl * 4U;
  packet[8U] = (uint8_t)(parsed.ttl - UINT8_C(1));
  packet[10U] = 0U;
  packet[11U] = 0U;
  checksum = tcpip_l03_checksum(packet, header_length);
  tcpip_l03_write_be16(packet, 10U, checksum);
  return TCPIP_L03_OK;
}
