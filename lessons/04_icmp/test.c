#include "lesson.h"

#include <tcpip/test.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

static const uint8_t tcpip_l04_payload[5] = {'a', 'b', 'c', 'd', 'e'};
static const uint8_t tcpip_l04_request[13] = {
    0x08U, 0x00U, 0xbbU, 0xfdU, 0x12U, 0x34U, 0x00U,
    0x07U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U};
static const uint8_t tcpip_l04_reply[13] = {
    0x00U, 0x00U, 0xc3U, 0xfdU, 0x12U, 0x34U, 0x00U,
    0x07U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U};
static const uint8_t tcpip_l04_error[8] = {
    0x03U, 0x01U, 0xfcU, 0xfeU, 0x00U, 0x00U, 0x00U, 0x00U};

static int tcpip_l04_is_student_stub(void) {
  tcpip_l04_message parsed;
  return tcpip_l04_parse_message(tcpip_l04_request, sizeof(tcpip_l04_request), &parsed) ==
         TCPIP_L04_TODO;
}

static void tcpip_l04_test_parse(tcpip_test_context *ctx) {
  tcpip_l04_message parsed;

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_parse_message(tcpip_l04_request, sizeof(tcpip_l04_request), &parsed),
      TCPIP_L04_OK);
  TCPIP_EXPECT_U32(ctx, parsed.type, TCPIP_L04_ECHO_REQUEST);
  TCPIP_EXPECT_U32(ctx, parsed.code, 0U);
  TCPIP_EXPECT_U16(ctx, parsed.checksum, 0xbbfdU);
  TCPIP_EXPECT_SIZE(ctx, parsed.body_offset, 4U);
  TCPIP_EXPECT_SIZE(ctx, parsed.body_length, 9U);

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_parse_message(tcpip_l04_error, sizeof(tcpip_l04_error), &parsed),
      TCPIP_L04_OK);
  TCPIP_EXPECT_U32(ctx, parsed.type, 3U);
  TCPIP_EXPECT_U32(ctx, parsed.code, 1U);
  TCPIP_EXPECT_U16(ctx, parsed.checksum, 0xfcfeU);
  TCPIP_EXPECT_SIZE(ctx, parsed.body_length, 4U);
}

static void tcpip_l04_test_parse_errors(tcpip_test_context *ctx) {
  tcpip_l04_message parsed;
  uint8_t corrupted[sizeof(tcpip_l04_request)];

  memset(&parsed, 0xa5, sizeof(parsed));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l04_parse_message(tcpip_l04_request, 3U, &parsed), TCPIP_L04_TRUNCATED);
  TCPIP_EXPECT_U32(ctx, parsed.type, 0U);
  TCPIP_EXPECT_SIZE(ctx, parsed.body_length, 0U);

  memcpy(corrupted, tcpip_l04_request, sizeof(corrupted));
  corrupted[12U] ^= 1U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_parse_message(corrupted, sizeof(corrupted), &parsed),
      TCPIP_L04_MALFORMED);
  TCPIP_EXPECT_U16(ctx, parsed.checksum, 0U);
  TCPIP_EXPECT_U32(ctx, tcpip_l04_parse_message(NULL, 0U, &parsed), TCPIP_L04_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_parse_message(tcpip_l04_request, sizeof(tcpip_l04_request), NULL),
      TCPIP_L04_INVALID_ARGUMENT);
}

static void tcpip_l04_test_build_exact(tcpip_test_context *ctx) {
  tcpip_l04_message parsed;
  uint8_t output[32];
  size_t output_length = 99U;

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_build_echo(
          TCPIP_L04_ECHO_REQUEST,
          0x1234U,
          7U,
          tcpip_l04_payload,
          sizeof(tcpip_l04_payload),
          output,
          sizeof(output),
          &output_length),
      TCPIP_L04_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, sizeof(tcpip_l04_request));
  TCPIP_EXPECT_BYTES(
      ctx, output, output_length, tcpip_l04_request, sizeof(tcpip_l04_request));
  TCPIP_EXPECT_U32(
      ctx, tcpip_l04_parse_message(output, output_length, &parsed), TCPIP_L04_OK);

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_build_echo(
          TCPIP_L04_ECHO_REPLY,
          0x1234U,
          7U,
          tcpip_l04_payload,
          sizeof(tcpip_l04_payload),
          output,
          sizeof(output),
          &output_length),
      TCPIP_L04_OK);
  TCPIP_EXPECT_BYTES(ctx, output, output_length, tcpip_l04_reply, sizeof(tcpip_l04_reply));
}

static void tcpip_l04_test_build_errors(tcpip_test_context *ctx) {
  uint8_t output[16];
  uint8_t expected[16];
  size_t output_length = 99U;

  memset(output, 0x5a, sizeof(output));
  memcpy(expected, output, sizeof(output));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_build_echo(
          TCPIP_L04_ECHO_REQUEST,
          1U,
          2U,
          tcpip_l04_payload,
          sizeof(tcpip_l04_payload),
          output,
          sizeof(tcpip_l04_request) - 1U,
          &output_length),
      TCPIP_L04_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), expected, sizeof(expected));

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_build_echo(
          3U, 1U, 2U, NULL, 0U, output, sizeof(output), &output_length),
      TCPIP_L04_MALFORMED);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_build_echo(
          TCPIP_L04_ECHO_REQUEST,
          1U,
          2U,
          NULL,
          1U,
          output,
          sizeof(output),
          &output_length),
      TCPIP_L04_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_build_echo(
          TCPIP_L04_ECHO_REQUEST,
          1U,
          2U,
          tcpip_l04_payload,
          (size_t)UINT16_MAX,
          output,
          sizeof(output),
          &output_length),
      TCPIP_L04_MALFORMED);
}

static void tcpip_l04_test_make_reply(tcpip_test_context *ctx) {
  uint8_t output[32];
  size_t output_length = 99U;

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(
          tcpip_l04_request,
          sizeof(tcpip_l04_request),
          output,
          sizeof(output),
          &output_length),
      TCPIP_L04_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, sizeof(tcpip_l04_reply));
  TCPIP_EXPECT_BYTES(ctx, output, output_length, tcpip_l04_reply, sizeof(tcpip_l04_reply));
}

static void tcpip_l04_test_reply_rejections(tcpip_test_context *ctx) {
  static const uint8_t nonzero_code[13] = {
      0x08U, 0x01U, 0xbbU, 0xfcU, 0x12U, 0x34U, 0x00U,
      0x07U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U};
  uint8_t output[32];
  uint8_t expected[32];
  uint8_t corrupted[sizeof(tcpip_l04_request)];
  size_t output_length = 99U;

  memset(output, 0xa5, sizeof(output));
  memcpy(expected, output, sizeof(output));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(
          tcpip_l04_reply, sizeof(tcpip_l04_reply), output, sizeof(output), &output_length),
      TCPIP_L04_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), expected, sizeof(expected));

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(
          tcpip_l04_error, sizeof(tcpip_l04_error), output, sizeof(output), &output_length),
      TCPIP_L04_MALFORMED);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(
          nonzero_code, sizeof(nonzero_code), output, sizeof(output), &output_length),
      TCPIP_L04_MALFORMED);

  memcpy(corrupted, tcpip_l04_request, sizeof(corrupted));
  corrupted[8U] ^= 1U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(
          corrupted, sizeof(corrupted), output, sizeof(output), &output_length),
      TCPIP_L04_MALFORMED);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(tcpip_l04_request, 3U, output, sizeof(output), &output_length),
      TCPIP_L04_TRUNCATED);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l04_make_echo_reply(
          tcpip_l04_request,
          sizeof(tcpip_l04_request),
          output,
          sizeof(tcpip_l04_request) - 1U,
          &output_length),
      TCPIP_L04_CAPACITY);
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 04: ICMP");
  if (tcpip_l04_is_student_stub()) {
    TCPIP_FAIL(&ctx, "lesson 04 exercise still returns TCPIP_L04_TODO");
    return tcpip_test_finish(&ctx);
  }
  tcpip_l04_test_parse(&ctx);
  tcpip_l04_test_parse_errors(&ctx);
  tcpip_l04_test_build_exact(&ctx);
  tcpip_l04_test_build_errors(&ctx);
  tcpip_l04_test_make_reply(&ctx);
  tcpip_l04_test_reply_rejections(&ctx);
  return tcpip_test_finish(&ctx);
}
