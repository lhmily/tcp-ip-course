#include "lesson.h"

#include <tcpip/test.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t tcpip_l05_test_source_ipv4[4] = {192U, 0U, 2U, 1U};
static const uint8_t tcpip_l05_test_destination_ipv4[4] = {198U, 51U, 100U, 2U};
static const uint8_t tcpip_l05_test_payload[5] = {'a', 'b', 'c', 'd', 'e'};
static const uint8_t tcpip_l05_test_datagram[13] = {
    0x30U, 0x39U, 0x00U, 0x35U, 0x00U, 0x0dU, 0xb9U,
    0x67U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U};

static int tcpip_l05_is_student_stub(void) {
  tcpip_l05_datagram parsed;
  return tcpip_l05_parse_datagram(
             tcpip_l05_test_datagram, sizeof(tcpip_l05_test_datagram), &parsed) ==
         TCPIP_L05_TODO;
}

static void tcpip_l05_test_parse(tcpip_test_context *ctx) {
  tcpip_l05_datagram parsed;
  uint8_t zero_checksum[sizeof(tcpip_l05_test_datagram)];

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_parse_datagram(
          tcpip_l05_test_datagram, sizeof(tcpip_l05_test_datagram), &parsed) == TCPIP_L05_OK);
  TCPIP_EXPECT_U16(ctx, parsed.source_port, 12345U);
  TCPIP_EXPECT_U16(ctx, parsed.destination_port, 53U);
  TCPIP_EXPECT_U16(ctx, parsed.length, 13U);
  TCPIP_EXPECT_U16(ctx, parsed.checksum, 0xb967U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 8U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 5U);

  memcpy(zero_checksum, tcpip_l05_test_datagram, sizeof(zero_checksum));
  zero_checksum[6] = 0U;
  zero_checksum[7] = 0U;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_parse_datagram(zero_checksum, sizeof(zero_checksum), &parsed) == TCPIP_L05_OK);
  TCPIP_EXPECT_U16(ctx, parsed.checksum, 0U);

  {
    const uint8_t empty_datagram[8] = {
        0x30U, 0x39U, 0xffU, 0xffU, 0x00U, 0x08U, 0x00U, 0x00U};

    memset(&parsed, 0xa5, sizeof(parsed));
    TCPIP_EXPECT_TRUE(
        ctx,
        tcpip_l05_parse_datagram(empty_datagram, sizeof(empty_datagram), &parsed) ==
            TCPIP_L05_OK);
    TCPIP_EXPECT_U16(ctx, parsed.source_port, 12345U);
    TCPIP_EXPECT_U16(ctx, parsed.destination_port, UINT16_MAX);
    TCPIP_EXPECT_U16(ctx, parsed.length, 8U);
    TCPIP_EXPECT_U16(ctx, parsed.checksum, 0U);
    TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 8U);
    TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 0U);
  }
}

static void tcpip_l05_test_parse_errors(tcpip_test_context *ctx) {
  tcpip_l05_datagram parsed;
  uint8_t short_length[8] = {0U, 1U, 0U, 2U, 0U, 7U, 0U, 0U};
  uint8_t long_length[9] = {0U, 1U, 0U, 2U, 0U, 10U, 0U, 0U, 0U};
  uint8_t trailing[14];

  memset(&parsed, 0xa5, sizeof(parsed));
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l05_parse_datagram(short_length, sizeof(short_length), &parsed) ==
               TCPIP_L05_MALFORMED);
  TCPIP_EXPECT_U16(ctx, parsed.source_port, 0U);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_length, 0U);

  memset(&parsed, 0xa5, sizeof(parsed));
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l05_parse_datagram(long_length, sizeof(long_length), &parsed) ==
               TCPIP_L05_TRUNCATED);
  TCPIP_EXPECT_SIZE(ctx, parsed.payload_offset, 0U);

  memcpy(trailing, tcpip_l05_test_datagram, sizeof(tcpip_l05_test_datagram));
  trailing[13] = 0U;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_parse_datagram(trailing, sizeof(trailing), &parsed) == TCPIP_L05_MALFORMED);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_parse_datagram(tcpip_l05_test_datagram, 7U, &parsed) ==
          TCPIP_L05_TRUNCATED);
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l05_parse_datagram(NULL, 0U, &parsed) == TCPIP_L05_INVALID_ARGUMENT);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_parse_datagram(tcpip_l05_test_datagram, sizeof(tcpip_l05_test_datagram), NULL) ==
          TCPIP_L05_INVALID_ARGUMENT);
}

static void tcpip_l05_test_checksum(tcpip_test_context *ctx) {
  uint8_t corrupted[sizeof(tcpip_l05_test_datagram)];
  uint16_t checksum = UINT16_C(0xffff);

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_ipv4_checksum(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          tcpip_l05_test_datagram,
          sizeof(tcpip_l05_test_datagram),
          &checksum) == TCPIP_L05_OK);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);

  memcpy(corrupted, tcpip_l05_test_datagram, sizeof(corrupted));
  corrupted[12] ^= UINT8_C(0x01);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_ipv4_checksum(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          corrupted,
          sizeof(corrupted),
          &checksum) == TCPIP_L05_OK);
  TCPIP_EXPECT_TRUE(ctx, checksum != 0U);

  checksum = UINT16_C(0xffff);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_ipv4_checksum(
          tcpip_l05_test_source_ipv4,
          3U,
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          corrupted,
          sizeof(corrupted),
          &checksum) == TCPIP_L05_INVALID_ARGUMENT);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);

  checksum = UINT16_C(0xffff);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_ipv4_checksum(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          corrupted,
          ((size_t)UINT16_MAX) + 1U,
          &checksum) == TCPIP_L05_MALFORMED);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);
}

static void tcpip_l05_test_build(tcpip_test_context *ctx) {
  uint8_t destination[32];
  size_t output_length = SIZE_MAX;
  tcpip_l05_datagram parsed;

  memset(destination, 0xa5, sizeof(destination));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          12345U,
          53U,
          tcpip_l05_test_payload,
          sizeof(tcpip_l05_test_payload),
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L05_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, sizeof(tcpip_l05_test_datagram));
  TCPIP_EXPECT_BYTES(
      ctx,
      destination,
      output_length,
      tcpip_l05_test_datagram,
      sizeof(tcpip_l05_test_datagram));
  TCPIP_EXPECT_TRUE(
      ctx, tcpip_l05_parse_datagram(destination, output_length, &parsed) == TCPIP_L05_OK);

  memset(destination, 0xa5, sizeof(destination));
  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          1U,
          UINT16_MAX,
          NULL,
          0U,
          destination,
          7U,
          &output_length) == TCPIP_L05_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  {
    const uint8_t unchanged[32] = {
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U};
    TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));
  }

  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          1U,
          2U,
          tcpip_l05_test_payload,
          (size_t)UINT16_MAX,
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L05_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  {
    const uint8_t unchanged[32] = {
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
        0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U};
    TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));
  }
}

static void tcpip_l05_test_zero_payload_build(tcpip_test_context *ctx) {
  const uint8_t expected[8] = {
      0x30U, 0x39U, 0xffU, 0xffU, 0x00U, 0x08U, 0xe3U, 0x6dU};
  uint8_t destination[8];
  size_t output_length = SIZE_MAX;

  memset(destination, 0xa5, sizeof(destination));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          12345U,
          UINT16_MAX,
          NULL,
          0U,
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L05_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, sizeof(expected));
  TCPIP_EXPECT_BYTES(ctx, destination, output_length, expected, sizeof(expected));
}

static void tcpip_l05_test_maximum_payload(tcpip_test_context *ctx) {
  const size_t payload_length = (size_t)UINT16_MAX - 8U;
  uint8_t *payload = (uint8_t *)malloc(payload_length);
  uint8_t *destination = (uint8_t *)malloc((size_t)UINT16_MAX);
  size_t output_length = SIZE_MAX;
  size_t index;

  if (payload == NULL || destination == NULL) {
    TCPIP_FAIL(ctx, "unable to allocate maximum-size UDP test buffers");
    free(destination);
    free(payload);
    return;
  }

  for (index = 0U; index < payload_length; ++index) {
    payload[index] = (uint8_t)(index & UINT8_MAX);
  }
  memset(destination, 0xa5, (size_t)UINT16_MAX);
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          1U,
          2U,
          payload,
          payload_length,
          destination,
          (size_t)UINT16_MAX,
          &output_length) == TCPIP_L05_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, (size_t)UINT16_MAX);
  TCPIP_EXPECT_TRUE(ctx, destination[4] == UINT8_C(0xff));
  TCPIP_EXPECT_TRUE(ctx, destination[5] == UINT8_C(0xff));
  TCPIP_EXPECT_BYTES(ctx, destination + 8U, payload_length, payload, payload_length);

  free(destination);
  free(payload);
}

static void tcpip_l05_test_build_ipv4_span_errors(tcpip_test_context *ctx) {
  static const uint8_t unchanged[16] = {
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U,
      0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U, 0xa5U};
  uint8_t destination[sizeof(unchanged)];
  size_t output_length = SIZE_MAX;

  memset(destination, 0xa5, sizeof(destination));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          3U,
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          1U,
          2U,
          NULL,
          0U,
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L05_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));

  output_length = SIZE_MAX;
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          5U,
          1U,
          2U,
          NULL,
          0U,
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L05_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));
}

static void tcpip_l05_test_zero_mapping(tcpip_test_context *ctx) {
  const uint8_t payload[2] = {0x13U, 0xa0U};
  const uint8_t expected[10] = {
      0x00U, 0x01U, 0x00U, 0x02U, 0x00U, 0x0aU, 0xffU, 0xffU, 0x13U, 0xa0U};
  uint8_t destination[10];
  size_t output_length = 0U;
  uint16_t checksum = UINT16_C(1);

  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_build_datagram(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          1U,
          2U,
          payload,
          sizeof(payload),
          destination,
          sizeof(destination),
          &output_length) == TCPIP_L05_OK);
  TCPIP_EXPECT_BYTES(ctx, destination, output_length, expected, sizeof(expected));
  TCPIP_EXPECT_TRUE(
      ctx,
      tcpip_l05_ipv4_checksum(
          tcpip_l05_test_source_ipv4,
          sizeof(tcpip_l05_test_source_ipv4),
          tcpip_l05_test_destination_ipv4,
          sizeof(tcpip_l05_test_destination_ipv4),
          destination,
          output_length,
          &checksum) == TCPIP_L05_OK);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 05 UDP datagrams");
  if (tcpip_l05_is_student_stub()) {
    TCPIP_FAIL(&ctx, "lesson 05 exercise still returns TCPIP_L05_TODO");
    return tcpip_test_finish(&ctx);
  }

  tcpip_l05_test_parse(&ctx);
  tcpip_l05_test_parse_errors(&ctx);
  tcpip_l05_test_checksum(&ctx);
  tcpip_l05_test_build(&ctx);
  tcpip_l05_test_zero_payload_build(&ctx);
  tcpip_l05_test_maximum_payload(&ctx);
  tcpip_l05_test_build_ipv4_span_errors(&ctx);
  tcpip_l05_test_zero_mapping(&ctx);
  return tcpip_test_finish(&ctx);
}
