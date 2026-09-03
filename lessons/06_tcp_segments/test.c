#include "lesson.h"

#include <tcpip/test.h>

#include <limits.h>
#include <string.h>

static const uint8_t tcpip_l06_test_client_ipv4[4] = {192U, 0U, 2U, 1U};
static const uint8_t tcpip_l06_test_server_ipv4[4] = {198U, 51U, 100U, 2U};
static const uint8_t tcpip_l06_test_options[8] = {
    0x02U, 0x04U, 0x05U, 0xb4U, 0x01U, 0x01U, 0x04U, 0x02U};
static const uint8_t tcpip_l06_test_syn[28] = {
    0xc0U, 0x00U, 0x00U, 0x50U, 0x12U, 0x34U, 0x56U, 0x78U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x70U, 0x02U, 0xfaU, 0xf0U,
    0x72U, 0xfaU, 0x00U, 0x00U, 0x02U, 0x04U, 0x05U, 0xb4U,
    0x01U, 0x01U, 0x04U, 0x02U};
static const uint8_t tcpip_l06_test_data[25] = {
    0x00U, 0x50U, 0xc0U, 0x00U, 0x89U, 0xabU, 0xcdU, 0xefU,
    0x12U, 0x34U, 0x56U, 0x79U, 0x51U, 0x18U, 0x10U, 0x00U,
    0xeeU, 0x24U, 0x00U, 0x00U, 0x68U, 0x65U, 0x6cU, 0x6cU, 0x6fU};

static int tcpip_l06_is_student_stub(void) {
  tcpip_l06_segment parsed;
  return tcpip_l06_parse_segment(
             tcpip_l06_test_syn, sizeof(tcpip_l06_test_syn), &parsed) == TCPIP_L06_TODO;
}

static tcpip_l06_segment_fields tcpip_l06_syn_fields(void) {
  tcpip_l06_segment_fields fields;

  fields.source_ipv4 = tcpip_l06_test_client_ipv4;
  fields.source_ipv4_length = sizeof(tcpip_l06_test_client_ipv4);
  fields.destination_ipv4 = tcpip_l06_test_server_ipv4;
  fields.destination_ipv4_length = sizeof(tcpip_l06_test_server_ipv4);
  fields.source_port = 49152U;
  fields.destination_port = 80U;
  fields.sequence_number = UINT32_C(0x12345678);
  fields.acknowledgment_number = 0U;
  fields.flags = TCPIP_L06_FLAG_SYN;
  fields.window = 64240U;
  fields.urgent_pointer = 0U;
  fields.options = tcpip_l06_test_options;
  fields.options_length = sizeof(tcpip_l06_test_options);
  return fields;
}

static void tcpip_l06_test_parse_syn(tcpip_test_context *ctx) {
  tcpip_l06_segment parsed;

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_parse_segment(tcpip_l06_test_syn, sizeof(tcpip_l06_test_syn), &parsed) ==
          TCPIP_L06_OK);
  TCPIP_EXPECT_U16(ctx, parsed.source_port, 49152U);
  TCPIP_EXPECT_U16(ctx, parsed.destination_port, 80U);
  TCPIP_EXPECT_U32(ctx, parsed.sequence_number, UINT32_C(0x12345678));
  TCPIP_EXPECT_U32(ctx, parsed.acknowledgment_number, 0U);
  TCPIP_EXPECT_U16(ctx, parsed.data_offset, 7U);
  TCPIP_EXPECT_U16(ctx, parsed.flags, TCPIP_L06_FLAG_SYN);
  TCPIP_EXPECT_U16(ctx, parsed.window, 64240U);
  TCPIP_EXPECT_U16(ctx, parsed.checksum, 0x72faU);
  TCPIP_EXPECT_U16(ctx, parsed.urgent_pointer, 0U);
  TCPIP_EXPECT_SIZE(ctx, parsed.options_offset, 20U);
  TCPIP_EXPECT_SIZE(ctx, parsed.options_length, 8U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 28U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 0U);
  TCPIP_EXPECT_BYTES(
      ctx,
      tcpip_l06_test_syn + parsed.options_offset,
      parsed.options_length,
      tcpip_l06_test_options,
      sizeof(tcpip_l06_test_options));
}

static void tcpip_l06_test_parse_data(tcpip_test_context *ctx) {
  tcpip_l06_segment parsed;
  const uint8_t expected_payload[5] = {'h', 'e', 'l', 'l', 'o'};

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_parse_segment(tcpip_l06_test_data, sizeof(tcpip_l06_test_data), &parsed) ==
          TCPIP_L06_OK);
  TCPIP_EXPECT_U16(ctx, parsed.source_port, 80U);
  TCPIP_EXPECT_U16(ctx, parsed.destination_port, 49152U);
  TCPIP_EXPECT_U32(ctx, parsed.sequence_number, UINT32_C(0x89abcdef));
  TCPIP_EXPECT_U32(ctx, parsed.acknowledgment_number, UINT32_C(0x12345679));
  TCPIP_EXPECT_U16(
      ctx, parsed.flags, TCPIP_L06_FLAG_NS | TCPIP_L06_FLAG_ACK | TCPIP_L06_FLAG_PSH);
  TCPIP_EXPECT_SIZE(ctx, parsed.options_length, 0U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 20U);
  TCPIP_EXPECT_BYTES(
      ctx,
      tcpip_l06_test_data + parsed.payload_offset,
      parsed.payload_length,
      expected_payload,
      sizeof(expected_payload));
}

static void tcpip_l06_test_parse_errors(tcpip_test_context *ctx) {
  tcpip_l06_segment parsed;
  uint8_t invalid[sizeof(tcpip_l06_test_syn)];

  memset(&parsed, 0xa5, sizeof(parsed));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_parse_segment(tcpip_l06_test_syn, 19U, &parsed) == TCPIP_L06_TRUNCATED);
  TCPIP_EXPECT_U16(ctx, parsed.source_port, 0U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 0U);

  memcpy(invalid, tcpip_l06_test_syn, sizeof(invalid));
  invalid[12] = UINT8_C(0x40);
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l06_parse_segment(invalid, sizeof(invalid), &parsed) == TCPIP_L06_MALFORMED);

  memcpy(invalid, tcpip_l06_test_syn, sizeof(invalid));
  invalid[12] = UINT8_C(0xf0);
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l06_parse_segment(invalid, sizeof(invalid), &parsed) == TCPIP_L06_TRUNCATED);

  memcpy(invalid, tcpip_l06_test_syn, sizeof(invalid));
  invalid[12] = UINT8_C(0x72);
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l06_parse_segment(invalid, sizeof(invalid), &parsed) == TCPIP_L06_MALFORMED);
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l06_parse_segment(NULL, 0U, &parsed) == TCPIP_L06_INVALID_ARGUMENT);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_parse_segment(tcpip_l06_test_syn, sizeof(tcpip_l06_test_syn), NULL) ==
          TCPIP_L06_INVALID_ARGUMENT);
}

static void tcpip_l06_test_checksum(tcpip_test_context *ctx) {
  uint8_t corrupted[sizeof(tcpip_l06_test_data)];
  uint16_t checksum = UINT16_MAX;

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_ipv4_checksum(
          tcpip_l06_test_client_ipv4,
          sizeof(tcpip_l06_test_client_ipv4),
          tcpip_l06_test_server_ipv4,
          sizeof(tcpip_l06_test_server_ipv4),
          tcpip_l06_test_syn,
          sizeof(tcpip_l06_test_syn),
          &checksum) == TCPIP_L06_OK);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_ipv4_checksum(
          tcpip_l06_test_server_ipv4,
          sizeof(tcpip_l06_test_server_ipv4),
          tcpip_l06_test_client_ipv4,
          sizeof(tcpip_l06_test_client_ipv4),
          tcpip_l06_test_data,
          sizeof(tcpip_l06_test_data),
          &checksum) == TCPIP_L06_OK);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);

  memcpy(corrupted, tcpip_l06_test_data, sizeof(corrupted));
  corrupted[24] ^= UINT8_C(0x80);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_ipv4_checksum(
          tcpip_l06_test_server_ipv4,
          sizeof(tcpip_l06_test_server_ipv4),
          tcpip_l06_test_client_ipv4,
          sizeof(tcpip_l06_test_client_ipv4),
          corrupted,
          sizeof(corrupted),
          &checksum) == TCPIP_L06_OK);
  TCPIP_EXPECT_TRUE(ctx, checksum != 0U);

  checksum = UINT16_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_ipv4_checksum(
          tcpip_l06_test_server_ipv4,
          3U,
          tcpip_l06_test_client_ipv4,
          sizeof(tcpip_l06_test_client_ipv4),
          corrupted,
          sizeof(corrupted),
          &checksum) == TCPIP_L06_INVALID_ARGUMENT);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);

  checksum = UINT16_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_ipv4_checksum(
          tcpip_l06_test_server_ipv4,
          sizeof(tcpip_l06_test_server_ipv4),
          tcpip_l06_test_client_ipv4,
          sizeof(tcpip_l06_test_client_ipv4),
          corrupted,
          ((size_t)UINT16_MAX) + 1U,
          &checksum) == TCPIP_L06_MALFORMED);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);
}

static void tcpip_l06_test_build(tcpip_test_context *ctx) {
  tcpip_l06_segment_fields fields = tcpip_l06_syn_fields();
  uint8_t destination[64];
  size_t output_length = SIZE_MAX;
  const uint8_t payload[5] = {'h', 'e', 'l', 'l', 'o'};

  memset(destination, 0xa5, sizeof(destination));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(
          &fields, NULL, 0U, destination, sizeof(destination), &output_length) == TCPIP_L06_OK);
  TCPIP_EXPECT_BYTES(
      ctx, destination, output_length, tcpip_l06_test_syn, sizeof(tcpip_l06_test_syn));

  fields.source_ipv4 = tcpip_l06_test_server_ipv4;
  fields.destination_ipv4 = tcpip_l06_test_client_ipv4;
  fields.source_port = 80U;
  fields.destination_port = 49152U;
  fields.sequence_number = UINT32_C(0x89abcdef);
  fields.acknowledgment_number = UINT32_C(0x12345679);
  fields.flags = TCPIP_L06_FLAG_NS | TCPIP_L06_FLAG_ACK | TCPIP_L06_FLAG_PSH;
  fields.window = 4096U;
  fields.options = NULL;
  fields.options_length = 0U;
  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(
          &fields,
          payload,
          sizeof(payload),
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L06_OK);
  TCPIP_EXPECT_BYTES(
      ctx, destination, output_length, tcpip_l06_test_data, sizeof(tcpip_l06_test_data));
}

static void tcpip_l06_test_build_errors(tcpip_test_context *ctx) {
  static const uint8_t unchanged[64] = {
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U};
  tcpip_l06_segment_fields fields = tcpip_l06_syn_fields();
  uint8_t destination[64];
  size_t output_length = SIZE_MAX;

  memset(destination, 0xa5, sizeof(destination));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(&fields, NULL, 0U, destination, 27U, &output_length) ==
          TCPIP_L06_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));

  fields.options_length = 6U;
  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(
          &fields, NULL, 0U, destination, sizeof(destination), &output_length) ==
          TCPIP_L06_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));

  fields.options_length = 44U;
  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(
          &fields, NULL, 0U, destination, sizeof(destination), &output_length) ==
          TCPIP_L06_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));

  fields = tcpip_l06_syn_fields();
  fields.flags = UINT16_C(0x0200);
  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(
          &fields, NULL, 0U, destination, sizeof(destination), &output_length) ==
          TCPIP_L06_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));

  fields = tcpip_l06_syn_fields();
  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l06_build_segment(
          &fields,
          tcpip_l06_test_options,
          (size_t)UINT16_MAX,
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L06_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));
}

static void tcpip_l06_test_sequence_arithmetic(tcpip_test_context *ctx) {
  TCPIP_EXPECT_TRUE(ctx, tcpip_l06_seq_before(10U, 11U));
  TCPIP_EXPECT_TRUE(ctx, !tcpip_l06_seq_before(11U, 10U));
  TCPIP_EXPECT_TRUE(ctx, !tcpip_l06_seq_before(10U, 10U));
  TCPIP_EXPECT_TRUE(ctx, tcpip_l06_seq_before(UINT32_MAX - 2U, 3U));
  TCPIP_EXPECT_TRUE(ctx, !tcpip_l06_seq_before(3U, UINT32_MAX - 2U));
  TCPIP_EXPECT_TRUE(ctx, !tcpip_l06_seq_before(0U, UINT32_C(0x80000000)));
  TCPIP_EXPECT_TRUE(ctx, !tcpip_l06_seq_before(UINT32_C(0x80000000), 0U));
  TCPIP_EXPECT_U32(ctx, tcpip_l06_seq_distance(UINT32_MAX - 2U, 3U), 6U);
  TCPIP_EXPECT_U32(ctx, tcpip_l06_seq_distance(3U, UINT32_MAX - 2U), UINT32_MAX - 5U);
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 06 TCP segments");
  if (tcpip_l06_is_student_stub()) {
    TCPIP_FAIL(&ctx, "lesson 06 exercise still returns TCPIP_L06_TODO");
    return tcpip_test_finish(&ctx);
  }

  tcpip_l06_test_parse_syn(&ctx);
  tcpip_l06_test_parse_data(&ctx);
  tcpip_l06_test_parse_errors(&ctx);
  tcpip_l06_test_checksum(&ctx);
  tcpip_l06_test_build(&ctx);
  tcpip_l06_test_build_errors(&ctx);
  tcpip_l06_test_sequence_arithmetic(&ctx);
  return tcpip_test_finish(&ctx);
}
