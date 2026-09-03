#include "lesson.h"

#include <tcpip/test.h>

#include <stdint.h>
#include <string.h>

static const uint8_t tcpip_l03_source[4] = {192U, 0U, 2U, 1U};
static const uint8_t tcpip_l03_destination[4] = {198U, 51U, 100U, 2U};
static const uint8_t tcpip_l03_packet[24] = {
    0x45U, 0x2eU, 0x00U, 0x18U, 0x12U, 0x34U, 0x40U, 0x00U,
    0x40U, 0x11U, 0x3cU, 0x3cU, 0xc0U, 0x00U, 0x02U, 0x01U,
    0xc6U, 0x33U, 0x64U, 0x02U, 0xdeU, 0xadU, 0xbeU, 0xefU};
static const uint8_t tcpip_l03_options_packet[28] = {
    0x46U, 0xa0U, 0x00U, 0x1cU, 0xbeU, 0xefU, 0x20U, 0x01U,
    0x20U, 0x06U, 0xa5U, 0x48U, 0x0aU, 0x00U, 0x00U, 0x01U,
    0x0aU, 0x00U, 0x00U, 0x02U, 0x01U, 0x01U, 0x00U, 0x00U,
    0xcaU, 0xfeU, 0xbaU, 0xbeU};

static int tcpip_l03_is_student_stub(void) {
  tcpip_l03_ipv4_packet parsed;
  return tcpip_l03_parse_ipv4(tcpip_l03_packet, sizeof(tcpip_l03_packet), &parsed) ==
         TCPIP_L03_TODO;
}

static void tcpip_l03_test_parse_base(tcpip_test_context *ctx) {
  tcpip_l03_ipv4_packet parsed;
  uint8_t containing[26];

  memcpy(containing, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  containing[24U] = 0xa5U;
  containing[25U] = 0x5aU;
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_parse_ipv4(containing, sizeof(containing), &parsed), TCPIP_L03_OK);
  TCPIP_EXPECT_U32(ctx, parsed.version, 4U);
  TCPIP_EXPECT_U32(ctx, parsed.ihl, 5U);
  TCPIP_EXPECT_U32(ctx, parsed.dscp_ecn, 0x2eU);
  TCPIP_EXPECT_U16(ctx, parsed.total_length, 24U);
  TCPIP_EXPECT_U16(ctx, parsed.identification, 0x1234U);
  TCPIP_EXPECT_U16(ctx, parsed.flags_fragment, 0x4000U);
  TCPIP_EXPECT_U32(ctx, parsed.ttl, 64U);
  TCPIP_EXPECT_U32(ctx, parsed.protocol, 17U);
  TCPIP_EXPECT_U16(ctx, parsed.header_checksum, 0x3c3cU);
  TCPIP_EXPECT_BYTES(ctx, parsed.source, sizeof(parsed.source), tcpip_l03_source, sizeof(tcpip_l03_source));
  TCPIP_EXPECT_BYTES(
      ctx, parsed.destination, sizeof(parsed.destination), tcpip_l03_destination, sizeof(tcpip_l03_destination));
  TCPIP_EXPECT_SIZE(ctx, parsed.options_offset, 20U);
  TCPIP_EXPECT_SIZE(ctx, parsed.options_length, 0U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 20U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 4U);
}

static void tcpip_l03_test_parse_options(tcpip_test_context *ctx) {
  static const uint8_t expected_source[4] = {10U, 0U, 0U, 1U};
  static const uint8_t expected_destination[4] = {10U, 0U, 0U, 2U};
  tcpip_l03_ipv4_packet parsed;

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l03_parse_ipv4(
          tcpip_l03_options_packet, sizeof(tcpip_l03_options_packet), &parsed),
      TCPIP_L03_OK);
  TCPIP_EXPECT_U32(ctx, parsed.ihl, 6U);
  TCPIP_EXPECT_U16(ctx, parsed.total_length, 28U);
  TCPIP_EXPECT_U16(ctx, parsed.identification, 0xbeefU);
  TCPIP_EXPECT_U16(ctx, parsed.flags_fragment, 0x2001U);
  TCPIP_EXPECT_U32(ctx, parsed.ttl, 32U);
  TCPIP_EXPECT_U32(ctx, parsed.protocol, 6U);
  TCPIP_EXPECT_BYTES(ctx, parsed.source, sizeof(parsed.source), expected_source, sizeof(expected_source));
  TCPIP_EXPECT_BYTES(
      ctx, parsed.destination, sizeof(parsed.destination), expected_destination, sizeof(expected_destination));
  TCPIP_EXPECT_SIZE(ctx, parsed.options_offset, 20U);
  TCPIP_EXPECT_SIZE(ctx, parsed.options_length, 4U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 24U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 4U);
}

static void tcpip_l03_expect_parse_failure(
    tcpip_test_context *ctx,
    const uint8_t *packet,
    size_t packet_length,
    tcpip_l03_status expected_status) {
  tcpip_l03_ipv4_packet parsed;
  tcpip_l03_ipv4_packet zero;

  memset(&parsed, UINT8_C(0xa5), sizeof(parsed));
  memset(&zero, 0, sizeof(zero));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_parse_ipv4(packet, packet_length, &parsed), expected_status);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&parsed,
      sizeof(parsed),
      (const uint8_t *)&zero,
      sizeof(zero));
}

static void tcpip_l03_test_parse_errors(tcpip_test_context *ctx) {
  tcpip_l03_ipv4_packet parsed;
  tcpip_l03_ipv4_packet zero;
  uint8_t changed[sizeof(tcpip_l03_options_packet)];

  tcpip_l03_expect_parse_failure(
      ctx, tcpip_l03_packet, 19U, TCPIP_L03_TRUNCATED);

  memcpy(changed, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  changed[0U] = 0x65U;
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_packet), TCPIP_L03_MALFORMED);

  memcpy(changed, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  changed[0U] = 0x44U;
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_packet), TCPIP_L03_MALFORMED);

  memcpy(changed, tcpip_l03_options_packet, sizeof(tcpip_l03_options_packet));
  changed[0U] = 0x48U;
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_options_packet), TCPIP_L03_TRUNCATED);

  memcpy(changed, tcpip_l03_options_packet, sizeof(tcpip_l03_options_packet));
  changed[2U] = 0x00U;
  changed[3U] = 0x14U;
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_options_packet), TCPIP_L03_MALFORMED);

  memcpy(changed, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  changed[2U] = 0x00U;
  changed[3U] = 0x19U;
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_packet), TCPIP_L03_TRUNCATED);

  memcpy(changed, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  changed[23U] ^= 0x01U;
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_parse_ipv4(changed, sizeof(tcpip_l03_packet), &parsed), TCPIP_L03_OK);

  memcpy(changed, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  changed[8U] ^= 0x01U;
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_packet), TCPIP_L03_MALFORMED);

  memcpy(changed, tcpip_l03_packet, sizeof(tcpip_l03_packet));
  changed[6U] |= UINT8_C(0x80);
  changed[10U] = UINT8_C(0xbc);
  changed[11U] = UINT8_C(0x3b);
  tcpip_l03_expect_parse_failure(
      ctx, changed, sizeof(tcpip_l03_packet), TCPIP_L03_MALFORMED);

  memset(&parsed, UINT8_C(0xa5), sizeof(parsed));
  memset(&zero, 0, sizeof(zero));
  TCPIP_EXPECT_U32(ctx, tcpip_l03_parse_ipv4(NULL, 0U, &parsed), TCPIP_L03_INVALID_ARGUMENT);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&parsed,
      sizeof(parsed),
      (const uint8_t *)&zero,
      sizeof(zero));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l03_parse_ipv4(tcpip_l03_packet, sizeof(tcpip_l03_packet), NULL),
      TCPIP_L03_INVALID_ARGUMENT);
}

static void tcpip_l03_test_build(tcpip_test_context *ctx) {
  static const uint8_t expected[20] = {
      0x45U, 0x2eU, 0x00U, 0x14U, 0x12U, 0x34U, 0x40U, 0x00U, 0x40U, 0x11U,
      0x3cU, 0x40U, 0xc0U, 0x00U, 0x02U, 0x01U, 0xc6U, 0x33U, 0x64U, 0x02U};
  tcpip_l03_ipv4_header_fields fields;
  tcpip_l03_ipv4_packet parsed;
  uint8_t output[64];
  uint8_t untouched[64];
  size_t output_length = 99U;

  memset(&fields, 0, sizeof(fields));
  fields.dscp_ecn = 0x2eU;
  fields.identification = 0x1234U;
  fields.flags_fragment = 0x4000U;
  fields.ttl = 64U;
  fields.protocol = 17U;
  memcpy(fields.source, tcpip_l03_source, sizeof(fields.source));
  memcpy(fields.destination, tcpip_l03_destination, sizeof(fields.destination));

  memset(output, 0xa5, sizeof(output));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l03_build_header(&fields, output, sizeof(output), &output_length),
      TCPIP_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, sizeof(expected));
  TCPIP_EXPECT_BYTES(ctx, output, output_length, expected, sizeof(expected));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_parse_ipv4(output, output_length, &parsed), TCPIP_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 0U);

  memset(output, 0x5a, sizeof(output));
  memcpy(untouched, output, sizeof(output));
  output_length = 99U;
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_build_header(&fields, output, 19U, &output_length), TCPIP_L03_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), untouched, sizeof(untouched));
}

static void tcpip_l03_expect_build_failure(
    tcpip_test_context *ctx,
    const tcpip_l03_ipv4_header_fields *fields,
    tcpip_l03_status expected_status) {
  uint8_t output[64];
  uint8_t untouched[64];
  size_t output_length = 99U;

  memset(output, UINT8_C(0x5a), sizeof(output));
  memcpy(untouched, output, sizeof(untouched));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l03_build_header(fields, output, sizeof(output), &output_length),
      expected_status);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), untouched, sizeof(untouched));
}

static void tcpip_l03_test_build_options(tcpip_test_context *ctx) {
  static const uint8_t options[4] = {0x01U, 0x01U, 0x00U, 0x00U};
  static const uint8_t expected[24] = {
      0x46U, 0xa0U, 0x00U, 0x18U, 0xbeU, 0xefU, 0x20U, 0x01U,
      0x20U, 0x06U, 0xa5U, 0x4cU, 0x0aU, 0x00U, 0x00U, 0x01U,
      0x0aU, 0x00U, 0x00U, 0x02U, 0x01U, 0x01U, 0x00U, 0x00U};
  tcpip_l03_ipv4_header_fields fields;
  uint8_t output[64];
  size_t output_length = 0U;

  memset(&fields, 0, sizeof(fields));
  fields.dscp_ecn = 0xa0U;
  fields.identification = 0xbeefU;
  fields.flags_fragment = 0x2001U;
  fields.ttl = 32U;
  fields.protocol = 6U;
  fields.source[0U] = 10U;
  fields.source[3U] = 1U;
  fields.destination[0U] = 10U;
  fields.destination[3U] = 2U;
  fields.options = options;
  fields.options_length = sizeof(options);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l03_build_header(&fields, output, sizeof(output), &output_length),
      TCPIP_L03_OK);
  TCPIP_EXPECT_BYTES(ctx, output, output_length, expected, sizeof(expected));

  fields.options_length = 3U;
  tcpip_l03_expect_build_failure(ctx, &fields, TCPIP_L03_MALFORMED);

  fields.options = NULL;
  fields.options_length = 4U;
  tcpip_l03_expect_build_failure(ctx, &fields, TCPIP_L03_MALFORMED);

  fields.options = options;
  fields.options_length = TCPIP_L03_MAX_OPTIONS_LENGTH + 1U;
  tcpip_l03_expect_build_failure(ctx, &fields, TCPIP_L03_MALFORMED);

  fields.options_length = 0U;
  fields.flags_fragment = 0x8000U;
  tcpip_l03_expect_build_failure(ctx, &fields, TCPIP_L03_MALFORMED);
}

static void tcpip_l03_test_decrement_ttl(tcpip_test_context *ctx) {
  static const uint8_t expected[24] = {
      0x45U, 0x2eU, 0x00U, 0x18U, 0x12U, 0x34U, 0x40U, 0x00U,
      0x3fU, 0x11U, 0x3dU, 0x3cU, 0xc0U, 0x00U, 0x02U, 0x01U,
      0xc6U, 0x33U, 0x64U, 0x02U, 0xdeU, 0xadU, 0xbeU, 0xefU};
  tcpip_l03_ipv4_packet parsed;
  uint8_t packet[sizeof(tcpip_l03_packet)];
  uint8_t before[sizeof(tcpip_l03_packet)];

  memcpy(packet, tcpip_l03_packet, sizeof(packet));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_decrement_ttl(packet, sizeof(packet)), TCPIP_L03_OK);
  TCPIP_EXPECT_BYTES(ctx, packet, sizeof(packet), expected, sizeof(expected));
  TCPIP_EXPECT_U32(ctx, tcpip_l03_parse_ipv4(packet, sizeof(packet), &parsed), TCPIP_L03_OK);
  TCPIP_EXPECT_U32(ctx, parsed.ttl, 63U);

  memcpy(packet, tcpip_l03_packet, sizeof(packet));
  packet[8U] = 1U;
  packet[10U] = 0x7bU;
  packet[11U] = 0x3cU;
  memcpy(before, packet, sizeof(before));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_decrement_ttl(packet, sizeof(packet)), TCPIP_L03_MALFORMED);
  TCPIP_EXPECT_BYTES(ctx, packet, sizeof(packet), before, sizeof(before));

  memcpy(packet, tcpip_l03_packet, sizeof(packet));
  packet[10U] ^= 1U;
  memcpy(before, packet, sizeof(before));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l03_decrement_ttl(packet, sizeof(packet)), TCPIP_L03_MALFORMED);
  TCPIP_EXPECT_BYTES(ctx, packet, sizeof(packet), before, sizeof(before));
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 03: IPv4 packets");
  if (tcpip_l03_is_student_stub()) {
    TCPIP_FAIL(&ctx, "lesson 03 exercise still returns TCPIP_L03_TODO");
    return tcpip_test_finish(&ctx);
  }
  tcpip_l03_test_parse_base(&ctx);
  tcpip_l03_test_parse_options(&ctx);
  tcpip_l03_test_parse_errors(&ctx);
  tcpip_l03_test_build(&ctx);
  tcpip_l03_test_build_options(&ctx);
  tcpip_l03_test_decrement_ttl(&ctx);
  return tcpip_test_finish(&ctx);
}
