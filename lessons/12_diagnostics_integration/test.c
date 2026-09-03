#include "lesson.h"
#include "tcpip/test.h"

#include <string.h>

#define FRAME_CAPACITY 400U
#define ETHERNET_LEN 14U
#define IPV4_LEN 20U

static void write_u16(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8U);
  data[1] = (uint8_t)(value & UINT16_C(0x00ff));
}

static void write_u32(uint8_t *data, uint32_t value) {
  data[0] = (uint8_t)(value >> 24U);
  data[1] = (uint8_t)((value >> 16U) & UINT32_C(0xff));
  data[2] = (uint8_t)((value >> 8U) & UINT32_C(0xff));
  data[3] = (uint8_t)(value & UINT32_C(0xff));
}

static uint32_t checksum_add(uint32_t sum, const uint8_t *data, size_t len) {
  size_t offset = 0U;

  while (len - offset >= 2U) {
    sum += (uint32_t)(((uint16_t)data[offset] << 8U) | (uint16_t)data[offset + 1U]);
    offset += 2U;
  }
  if (offset < len) {
    sum += (uint32_t)data[offset] << 8U;
  }
  return sum;
}

static uint16_t checksum_complete(uint32_t sum) {
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)~sum;
}

static uint16_t internet_checksum(const uint8_t *data, size_t len) {
  return checksum_complete(checksum_add(0U, data, len));
}

static uint16_t transport_checksum(
    const uint8_t *ipv4, uint8_t protocol, const uint8_t *segment, size_t segment_len) {
  uint32_t sum = checksum_add(0U, ipv4 + 12U, 8U);

  sum += (uint32_t)protocol;
  sum += (uint32_t)segment_len;
  sum = checksum_add(sum, segment, segment_len);
  return checksum_complete(sum);
}

static void begin_ethernet(uint8_t *frame, uint16_t ether_type) {
  const uint8_t destination[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};
  const uint8_t source[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

  memcpy(frame, destination, sizeof(destination));
  memcpy(frame + 6U, source, sizeof(source));
  write_u16(frame + 12U, ether_type);
}

static uint8_t *begin_ipv4(uint8_t *frame, uint8_t protocol, size_t payload_len) {
  uint8_t *ipv4 = frame + ETHERNET_LEN;

  begin_ethernet(frame, UINT16_C(0x0800));
  ipv4[0] = UINT8_C(0x45);
  ipv4[1] = 0U;
  write_u16(ipv4 + 2U, (uint16_t)(IPV4_LEN + payload_len));
  write_u16(ipv4 + 4U, UINT16_C(0x1234));
  write_u16(ipv4 + 6U, UINT16_C(0x4000));
  ipv4[8] = 64U;
  ipv4[9] = protocol;
  write_u16(ipv4 + 10U, 0U);
  write_u32(ipv4 + 12U, UINT32_C(0xc0000201));
  write_u32(ipv4 + 16U, UINT32_C(0xc6336402));
  write_u16(ipv4 + 10U, internet_checksum(ipv4, IPV4_LEN));
  return ipv4;
}

static size_t build_arp(uint8_t *frame) {
  uint8_t *arp;

  memset(frame, 0, FRAME_CAPACITY);
  begin_ethernet(frame, UINT16_C(0x0806));
  arp = frame + ETHERNET_LEN;
  write_u16(arp, 1U);
  write_u16(arp + 2U, UINT16_C(0x0800));
  arp[4] = 6U;
  arp[5] = 4U;
  write_u16(arp + 6U, 1U);
  memcpy(arp + 8U, frame + 6U, 6U);
  write_u32(arp + 14U, UINT32_C(0xc0000201));
  memset(arp + 18U, 0, 6U);
  write_u32(arp + 24U, UINT32_C(0xc0000202));
  return ETHERNET_LEN + 28U;
}

static size_t build_icmp(uint8_t *frame) {
  uint8_t *ipv4;
  uint8_t *icmp;
  const size_t icmp_len = 12U;

  memset(frame, 0, FRAME_CAPACITY);
  ipv4 = begin_ipv4(frame, 1U, icmp_len);
  icmp = ipv4 + IPV4_LEN;
  icmp[0] = 8U;
  icmp[1] = 0U;
  write_u16(icmp + 4U, UINT16_C(0xbeef));
  write_u16(icmp + 6U, 1U);
  memcpy(icmp + 8U, "ping", 4U);
  write_u16(icmp + 2U, internet_checksum(icmp, icmp_len));
  return ETHERNET_LEN + IPV4_LEN + icmp_len;
}

static size_t build_dns(uint8_t *frame) {
  uint8_t *ipv4;
  uint8_t *udp;
  uint8_t *dns;
  const uint8_t question[] = {
      7U, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3U, 'c', 'o', 'm', 0U, 0U, 1U, 0U, 1U};
  const size_t dns_len = 12U + sizeof(question);
  const size_t udp_len = 8U + dns_len;
  uint16_t sum;

  memset(frame, 0, FRAME_CAPACITY);
  ipv4 = begin_ipv4(frame, 17U, udp_len);
  udp = ipv4 + IPV4_LEN;
  write_u16(udp, UINT16_C(53000));
  write_u16(udp + 2U, 53U);
  write_u16(udp + 4U, (uint16_t)udp_len);
  dns = udp + 8U;
  write_u16(dns, UINT16_C(0x1a2b));
  write_u16(dns + 2U, UINT16_C(0x0100));
  write_u16(dns + 4U, 1U);
  memcpy(dns + 12U, question, sizeof(question));
  sum = transport_checksum(ipv4, 17U, udp, udp_len);
  write_u16(udp + 6U, sum == 0U ? UINT16_C(0xffff) : sum);
  return ETHERNET_LEN + IPV4_LEN + udp_len;
}

static size_t build_tcp(
    uint8_t *frame,
    uint16_t source_port,
    uint16_t destination_port,
    const uint8_t *payload,
    size_t payload_len) {
  uint8_t *ipv4;
  uint8_t *tcp;
  const size_t tcp_len = 20U + payload_len;

  memset(frame, 0, FRAME_CAPACITY);
  ipv4 = begin_ipv4(frame, 6U, tcp_len);
  tcp = ipv4 + IPV4_LEN;
  write_u16(tcp, source_port);
  write_u16(tcp + 2U, destination_port);
  write_u32(tcp + 4U, UINT32_C(0x01020304));
  write_u32(tcp + 8U, UINT32_C(0x05060708));
  tcp[12] = UINT8_C(0x50);
  tcp[13] = UINT8_C(0x18);
  write_u16(tcp + 14U, UINT16_C(4096));
  if (payload_len != 0U) {
    memcpy(tcp + 20U, payload, payload_len);
  }
  write_u16(tcp + 16U, transport_checksum(ipv4, 6U, tcp, tcp_len));
  return ETHERNET_LEN + IPV4_LEN + tcp_len;
}

static size_t build_http(uint8_t *frame) {
  const uint8_t message[] = "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
  return build_tcp(frame, UINT16_C(49152), 80U, message, sizeof(message) - 1U);
}

static void expect_path(
    tcpip_test_context *ctx,
    const tcpip_l12_report *report,
    const tcpip_l12_layer *expected,
    size_t expected_count) {
  size_t index;

  TCPIP_EXPECT_SIZE(ctx, report->layer_count, expected_count);
  if (report->layer_count != expected_count) {
    return;
  }
  for (index = 0U; index < expected_count; index += 1U) {
    TCPIP_EXPECT_U32(ctx, report->layers[index], expected[index]);
  }
}

static void test_invalid_arguments(tcpip_test_context *ctx) {
  tcpip_l12_report report;
  uint8_t byte = 0U;
  char output[8];
  size_t written = 99U;

  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(&byte, 1U, NULL), TCPIP_L12_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(NULL, 1U, &report), TCPIP_L12_INVALID_ARGUMENT);
  memset(output, 'x', sizeof(output));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l12_format_report(NULL, output, sizeof(output), &written),
      TCPIP_L12_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(ctx, written, 0U);
  TCPIP_EXPECT_U32(ctx, (uint8_t)output[0], (uint8_t)'x');

  written = 99U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l12_format_report(&report, NULL, sizeof(output), &written),
      TCPIP_L12_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(ctx, written, 0U);
}

static void test_arp(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  const tcpip_l12_layer path[] = {TCPIP_L12_LAYER_ETHERNET, TCPIP_L12_LAYER_ARP};
  size_t len = build_arp(frame);

  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);
  expect_path(ctx, &report, path, sizeof(path) / sizeof(path[0]));
  TCPIP_EXPECT_U32(ctx, report.diagnostics, TCPIP_L12_DIAG_NONE);
  TCPIP_EXPECT_U16(ctx, report.ether_type, UINT16_C(0x0806));
  TCPIP_EXPECT_U16(ctx, report.arp_opcode, 1U);
  TCPIP_EXPECT_SIZE(ctx, report.parsed_length, len);
}

static void test_icmp(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  const tcpip_l12_layer path[] = {
      TCPIP_L12_LAYER_ETHERNET, TCPIP_L12_LAYER_IPV4, TCPIP_L12_LAYER_ICMP};
  size_t len = build_icmp(frame);

  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);
  expect_path(ctx, &report, path, sizeof(path) / sizeof(path[0]));
  TCPIP_EXPECT_U32(ctx, report.diagnostics, TCPIP_L12_DIAG_NONE);
  TCPIP_EXPECT_U32(ctx, report.icmp_type, 8U);
  TCPIP_EXPECT_U32(ctx, report.checksums_checked, 2U);
  TCPIP_EXPECT_U32(ctx, report.checksums_valid, 2U);
}

static void test_dns_and_format(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  const tcpip_l12_layer path[] = {
      TCPIP_L12_LAYER_ETHERNET,
      TCPIP_L12_LAYER_IPV4,
      TCPIP_L12_LAYER_UDP,
      TCPIP_L12_LAYER_DNS};
  char output[512];
  char short_output[12];
  size_t written = 0U;
  size_t len = build_dns(frame);

  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);
  expect_path(ctx, &report, path, sizeof(path) / sizeof(path[0]));
  TCPIP_EXPECT_U16(ctx, report.dns_identifier, UINT16_C(0x1a2b));
  TCPIP_EXPECT_U16(ctx, report.dns_question_count, 1U);
  TCPIP_EXPECT_U16(ctx, report.source_port, UINT16_C(53000));
  TCPIP_EXPECT_U16(ctx, report.destination_port, 53U);
  TCPIP_EXPECT_U32(ctx, report.checksums_valid, 2U);

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l12_format_report(&report, output, sizeof(output), &written),
      TCPIP_L12_OK);
  TCPIP_EXPECT_SIZE(ctx, written, strlen(output));
  TCPIP_EXPECT_CSTR(
      ctx,
      output,
      "layers=ethernet>ipv4>udp>dns diagnostics=0x00000000 frame=71 parsed=71 "
      "payload=29 ethertype=0x0800 arp_opcode=0 ip_protocol=17 "
      "src=192.0.2.1 dst=198.51.100.2 ports=53000->53 icmp_type=0 "
      "dns_id=0x1a2b dns_questions=1 tcp_flags=0x00 http_bytes=0 checksums=2/2");
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l12_format_report(&report, short_output, sizeof(short_output), &written),
      TCPIP_L12_CAPACITY);
  TCPIP_EXPECT_TRUE(ctx, short_output[sizeof(short_output) - 1U] == '\0');
  TCPIP_EXPECT_TRUE(ctx, written > sizeof(short_output));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_format_report(&report, NULL, 0U, &written), TCPIP_L12_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, written, strlen(output));

  report.layer_count = TCPIP_L12_MAX_LAYERS + 1U;
  written = 99U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l12_format_report(&report, output, sizeof(output), &written),
      TCPIP_L12_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(ctx, written, 0U);
}

static void test_http_and_flipped_payload(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  const tcpip_l12_layer path[] = {
      TCPIP_L12_LAYER_ETHERNET,
      TCPIP_L12_LAYER_IPV4,
      TCPIP_L12_LAYER_TCP,
      TCPIP_L12_LAYER_HTTP};
  size_t len = build_http(frame);

  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);
  expect_path(ctx, &report, path, sizeof(path) / sizeof(path[0]));
  TCPIP_EXPECT_U32(ctx, report.diagnostics, TCPIP_L12_DIAG_NONE);
  TCPIP_EXPECT_U32(ctx, report.tcp_flags, UINT8_C(0x18));
  TCPIP_EXPECT_SIZE(ctx, report.http_message_length, len - 54U);
  TCPIP_EXPECT_U32(ctx, report.checksums_valid, 2U);

  frame[64] ^= UINT8_C(0x01);
  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);
  expect_path(ctx, &report, path, sizeof(path) / sizeof(path[0]));
  TCPIP_EXPECT_TRUE(
      ctx, (report.diagnostics & TCPIP_L12_DIAG_CHECKSUM_MISMATCH) != 0U);
  TCPIP_EXPECT_U32(ctx, report.checksums_checked, 2U);
  TCPIP_EXPECT_U32(ctx, report.checksums_valid, 1U);
}

static size_t build_dns_payload_frame(
    uint8_t *frame, const uint8_t *dns_payload, size_t dns_len) {
  uint8_t *ipv4;
  uint8_t *udp;
  const size_t udp_len = 8U + dns_len;
  uint16_t sum;

  memset(frame, 0, FRAME_CAPACITY);
  ipv4 = begin_ipv4(frame, 17U, udp_len);
  udp = ipv4 + IPV4_LEN;
  write_u16(udp, UINT16_C(53000));
  write_u16(udp + 2U, 53U);
  write_u16(udp + 4U, (uint16_t)udp_len);
  memcpy(udp + 8U, dns_payload, dns_len);
  sum = transport_checksum(ipv4, 17U, udp, udp_len);
  write_u16(udp + 6U, sum == 0U ? UINT16_C(0xffff) : sum);
  return ETHERNET_LEN + IPV4_LEN + udp_len;
}

static void test_udp_exact_length(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  size_t len = build_dns(frame);
  uint8_t *ipv4 = frame + ETHERNET_LEN;
  uint8_t *udp = ipv4 + IPV4_LEN;

  write_u16(udp + 4U, (uint16_t)(len - ETHERNET_LEN - IPV4_LEN - 1U));
  write_u16(udp + 6U, 0U);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);
  TCPIP_EXPECT_TRUE(ctx, (report.diagnostics & TCPIP_L12_DIAG_MALFORMED) != 0U);
  TCPIP_EXPECT_SIZE(ctx, report.parsed_length, len - 1U);
}

static void test_dns_declared_records(tcpip_test_context *ctx) {
  static const uint8_t complete[] = {
      0x12U, 0x34U, 0x81U, 0x80U, 0x00U, 0x01U, 0x00U, 0x01U,
      0x00U, 0x01U, 0x00U, 0x01U,
      0x01U, 'a', 0x00U, 0x00U, 0x01U, 0x00U, 0x01U,
      0xc0U, 0x0cU, 0x00U, 0x01U, 0x00U, 0x01U, 0x00U, 0x00U,
      0x00U, 0x3cU, 0x00U, 0x04U, 192U, 0U, 2U, 1U,
      0xc0U, 0x0cU, 0x00U, 0x02U, 0x00U, 0x01U, 0x00U, 0x00U,
      0x00U, 0x3cU, 0x00U, 0x02U, 0xc0U, 0x0cU,
      0xc0U, 0x0cU, 0x00U, 0x10U, 0x00U, 0x01U, 0x00U, 0x00U,
      0x00U, 0x3cU, 0x00U, 0x01U, 0x00U};
  uint8_t frame[FRAME_CAPACITY];
  uint8_t malformed[sizeof(complete)];
  tcpip_l12_report report;
  size_t len = build_dns_payload_frame(frame, complete, sizeof(complete));

  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);

  memcpy(malformed, complete, sizeof(malformed));
  malformed[60] = 2U;
  len = build_dns_payload_frame(frame, malformed, sizeof(malformed));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_TRUNCATED);

  memcpy(malformed, complete, sizeof(malformed));
  malformed[20] = 19U;
  len = build_dns_payload_frame(frame, malformed, sizeof(malformed));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);

  memcpy(malformed, complete, sizeof(malformed));
  malformed[20] = 35U;
  malformed[35] = UINT8_C(0xc0);
  malformed[36] = 19U;
  len = build_dns_payload_frame(frame, malformed, sizeof(malformed));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);

  len = build_dns_payload_frame(frame, complete, sizeof(complete) - 1U);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_TRUNCATED);
}

static void test_dns_name_validation(tcpip_test_context *ctx) {
  uint8_t dns[12U + 257U + 4U];
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  size_t cursor;
  size_t len;

  memset(dns, 0, sizeof(dns));
  write_u16(dns + 4U, 1U);
  dns[12] = UINT8_C(0xc0);
  dns[13] = 12U;
  len = build_dns_payload_frame(frame, dns, 18U);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);

  dns[13] = 14U;
  dns[14] = UINT8_C(0xc0);
  dns[15] = 12U;
  len = build_dns_payload_frame(frame, dns, 20U);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);

  memset(dns, 0, sizeof(dns));
  write_u16(dns + 4U, 1U);
  cursor = 12U;
  for (size_t label = 0U; label < 4U; label += 1U) {
    dns[cursor] = 63U;
    memset(dns + cursor + 1U, 'a', 63U);
    cursor += 64U;
  }
  dns[cursor] = 0U;
  cursor += 1U;
  write_u16(dns + cursor, 1U);
  write_u16(dns + cursor + 2U, 1U);
  cursor += 4U;
  len = build_dns_payload_frame(frame, dns, cursor);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);
}

static void test_tcp_and_ipv4_reserved_fields(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  const tcpip_l12_layer tcp_path[] = {
      TCPIP_L12_LAYER_ETHERNET, TCPIP_L12_LAYER_IPV4, TCPIP_L12_LAYER_TCP};
  size_t len = build_tcp(frame, UINT16_C(49152), 80U, NULL, 0U);
  uint8_t *ipv4 = frame + ETHERNET_LEN;
  uint8_t *tcp = ipv4 + IPV4_LEN;

  TCPIP_EXPECT_U32(ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_OK);
  expect_path(ctx, &report, tcp_path, sizeof(tcp_path) / sizeof(tcp_path[0]));
  TCPIP_EXPECT_SIZE(ctx, report.payload_length, 0U);
  TCPIP_EXPECT_SIZE(ctx, report.http_message_length, 0U);
  TCPIP_EXPECT_U32(ctx, report.diagnostics, TCPIP_L12_DIAG_UNSUPPORTED);

  tcp[12] |= UINT8_C(0x02);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);

  len = build_tcp(frame, UINT16_C(49152), 80U, NULL, 0U);
  ipv4 = frame + ETHERNET_LEN;
  write_u16(ipv4 + 6U, UINT16_C(0xc000));
  write_u16(ipv4 + 10U, 0U);
  write_u16(ipv4 + 10U, internet_checksum(ipv4, IPV4_LEN));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);
}

static void test_diagnostics(tcpip_test_context *ctx) {
  uint8_t frame[FRAME_CAPACITY];
  tcpip_l12_report report;
  const tcpip_l12_layer truncated_path[] = {
      TCPIP_L12_LAYER_ETHERNET, TCPIP_L12_LAYER_IPV4};
  size_t len = build_dns(frame);

  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, 18U, &report), TCPIP_L12_TRUNCATED);
  expect_path(
      ctx, &report, truncated_path, sizeof(truncated_path) / sizeof(truncated_path[0]));
  TCPIP_EXPECT_TRUE(ctx, (report.diagnostics & TCPIP_L12_DIAG_TRUNCATED) != 0U);

  write_u16(frame + ETHERNET_LEN + 2U, 19U);
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, len, &report), TCPIP_L12_MALFORMED);
  TCPIP_EXPECT_TRUE(ctx, (report.diagnostics & TCPIP_L12_DIAG_MALFORMED) != 0U);

  memset(frame, 0, FRAME_CAPACITY);
  begin_ethernet(frame, UINT16_C(0x86dd));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l12_diagnose_frame(frame, ETHERNET_LEN, &report), TCPIP_L12_OK);
  TCPIP_EXPECT_TRUE(ctx, (report.diagnostics & TCPIP_L12_DIAG_UNSUPPORTED) != 0U);
  TCPIP_EXPECT_SIZE(ctx, report.layer_count, 1U);
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 12 diagnostics integration");
  test_invalid_arguments(&ctx);
  test_arp(&ctx);
  test_icmp(&ctx);
  test_dns_and_format(&ctx);
  test_http_and_flipped_payload(&ctx);
  test_udp_exact_length(&ctx);
  test_dns_declared_records(&ctx);
  test_dns_name_validation(&ctx);
  test_tcp_and_ipv4_reserved_fields(&ctx);
  test_diagnostics(&ctx);
  return tcpip_test_finish(&ctx);
}
