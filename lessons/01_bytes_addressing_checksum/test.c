#include "lesson.h"

#include <tcpip/test.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void test_big_endian_access(tcpip_test_context *ctx) {
  const uint8_t source[] = {UINT8_C(0xaa), UINT8_C(0x12), UINT8_C(0x34), UINT8_C(0xbb)};
  uint8_t destination[] = {UINT8_C(0x55), UINT8_C(0x55), UINT8_C(0x55), UINT8_C(0x55)};
  const uint8_t expected[] = {UINT8_C(0x55), UINT8_C(0xab), UINT8_C(0xcd), UINT8_C(0x55)};
  const uint8_t unchanged[] = {UINT8_C(0x55), UINT8_C(0xab), UINT8_C(0xcd), UINT8_C(0x55)};
  uint16_t value = UINT16_C(0xffff);

  TCPIP_EXPECT_U32(ctx, tcpip_l01_read_be16(source, sizeof(source), 1U, &value), TCPIP_L01_OK);
  TCPIP_EXPECT_U16(ctx, value, UINT16_C(0x1234));
  TCPIP_EXPECT_U32(ctx, tcpip_l01_write_be16(destination, sizeof(destination), 1U, UINT16_C(0xabcd)), TCPIP_L01_OK);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), expected, sizeof(expected));

  value = UINT16_C(0xffff);
  TCPIP_EXPECT_U32(ctx, tcpip_l01_read_be16(source, sizeof(source), 3U, &value), TCPIP_L01_TRUNCATED);
  TCPIP_EXPECT_U16(ctx, value, 0U);
  TCPIP_EXPECT_U32(ctx, tcpip_l01_write_be16(destination, sizeof(destination), 3U, UINT16_C(0x0000)), TCPIP_L01_CAPACITY);
  TCPIP_EXPECT_BYTES(ctx, destination, sizeof(destination), unchanged, sizeof(unchanged));
}

static void test_ipv4_parser(tcpip_test_context *ctx) {
  const char valid[] = {'1', '9', '2', '.', '0', '.', '2', '.', '1'};
  const uint8_t expected[] = {UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(1)};
  const char *invalid[] = {"", "192.0.2", "192..2.1", "256.0.2.1", "1.2.3.4x", "1.2.3.4.5"};
  uint8_t address[4] = {UINT8_C(9), UINT8_C(9), UINT8_C(9), UINT8_C(9)};
  const uint8_t zero[4] = {0U, 0U, 0U, 0U};
  size_t index = 0U;

  TCPIP_EXPECT_U32(ctx, tcpip_l01_parse_ipv4(valid, sizeof(valid), address), TCPIP_L01_OK);
  TCPIP_EXPECT_BYTES(ctx, address, sizeof(address), expected, sizeof(expected));

  for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
    memset(address, UINT8_C(0xa5), sizeof(address));
    TCPIP_EXPECT_U32(
        ctx, tcpip_l01_parse_ipv4(invalid[index], strlen(invalid[index]), address), TCPIP_L01_MALFORMED);
    TCPIP_EXPECT_BYTES(ctx, address, sizeof(address), zero, sizeof(zero));
  }
}

static void test_prefixes(tcpip_test_context *ctx) {
  const uint8_t address[] = {UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(129)};
  const uint8_t same[] = {UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(129)};
  const uint8_t subnet[] = {UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(128)};
  const uint8_t other[] = {UINT8_C(192), UINT8_C(0), UINT8_C(3), UINT8_C(129)};
  bool contains = false;

  TCPIP_EXPECT_U32(ctx, tcpip_l01_prefix_contains(address, other, 0U, &contains), TCPIP_L01_OK);
  TCPIP_EXPECT_TRUE(ctx, contains);
  TCPIP_EXPECT_U32(ctx, tcpip_l01_prefix_contains(address, same, 32U, &contains), TCPIP_L01_OK);
  TCPIP_EXPECT_TRUE(ctx, contains);
  TCPIP_EXPECT_U32(ctx, tcpip_l01_prefix_contains(address, subnet, 25U, &contains), TCPIP_L01_OK);
  TCPIP_EXPECT_TRUE(ctx, contains);
  contains = true;
  TCPIP_EXPECT_U32(ctx, tcpip_l01_prefix_contains(address, other, 32U, &contains), TCPIP_L01_OK);
  TCPIP_EXPECT_TRUE(ctx, !contains);
  contains = true;
  TCPIP_EXPECT_U32(ctx, tcpip_l01_prefix_contains(address, same, 33U, &contains), TCPIP_L01_INVALID_ARGUMENT);
  TCPIP_EXPECT_TRUE(ctx, !contains);
}

static void test_checksums(tcpip_test_context *ctx) {
  const uint8_t even[] = {
      UINT8_C(0x00), UINT8_C(0x01), UINT8_C(0xf2), UINT8_C(0x03),
      UINT8_C(0xf4), UINT8_C(0xf5), UINT8_C(0xf6), UINT8_C(0xf7)};
  const uint8_t odd[] = {UINT8_C(0x01), UINT8_C(0x02), UINT8_C(0x03)};
  uint16_t checksum = 0U;

  TCPIP_EXPECT_U32(ctx, tcpip_l01_checksum16(NULL, 0U, &checksum), TCPIP_L01_OK);
  TCPIP_EXPECT_U16(ctx, checksum, UINT16_C(0xffff));
  TCPIP_EXPECT_U32(ctx, tcpip_l01_checksum16(even, sizeof(even), &checksum), TCPIP_L01_OK);
  TCPIP_EXPECT_U16(ctx, checksum, UINT16_C(0x220d));
  TCPIP_EXPECT_U32(ctx, tcpip_l01_checksum16(odd, sizeof(odd), &checksum), TCPIP_L01_OK);
  TCPIP_EXPECT_U16(ctx, checksum, UINT16_C(0xfbfd));

  checksum = UINT16_C(0xffff);
  TCPIP_EXPECT_U32(ctx, tcpip_l01_checksum16(NULL, 1U, &checksum), TCPIP_L01_INVALID_ARGUMENT);
  TCPIP_EXPECT_U16(ctx, checksum, 0U);
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 01: bytes, addressing, and checksum");
  test_big_endian_access(&ctx);
  test_ipv4_parser(&ctx);
  test_prefixes(&ctx);
  test_checksums(&ctx);
  return tcpip_test_finish(&ctx);
}
