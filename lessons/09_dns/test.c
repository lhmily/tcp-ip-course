#include "lesson.h"
#include "tcpip/test.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint8_t tcpip_l09_query[] = {
    0x12U, 0x34U, 0x01U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x07U, 'e',   'x',   'a',
    'm',   'p',   'l',   'e',   0x03U, 'c',   'o',   'm',
    0x00U, 0x00U, 0x01U, 0x00U, 0x01U};

static const uint8_t tcpip_l09_response[] = {
    0xbeU, 0xefU, 0x81U, 0x80U, 0x00U, 0x01U, 0x00U, 0x01U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x07U, 'e',   'x',   'a',
    'm',   'p',   'l',   'e',   0x03U, 'c',   'o',   'm',
    0x00U, 0x00U, 0x01U, 0x00U, 0x01U, 0xc0U, 0x0cU, 0x00U,
    0x01U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x3cU, 0x00U,
    0x04U, 192U,  0U,    2U,    1U};

static const uint8_t tcpip_l09_nxdomain[] = {
    0xbeU, 0xefU, 0x81U, 0x83U, 0x00U, 0x01U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x07U, 'e',   'x',   'a',
    'm',   'p',   'l',   'e',   0x03U, 'c',   'o',   'm',
    0x00U, 0x00U, 0x01U, 0x00U, 0x01U};

static void test_encode_name(tcpip_test_context *test) {
  static const uint8_t expected[] = {
      0x03U, 'w', 'w', 'w', 0x07U, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
      0x03U, 'c', 'o', 'm', 0x00U};
  uint8_t output[32];
  size_t written = 99U;

  memset(output, 0xa5, sizeof(output));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_encode_name("www.example.com", 15U, output, sizeof(output), &written),
      TCPIP_L09_OK);
  TCPIP_EXPECT_SIZE(test, written, sizeof(expected));
  TCPIP_EXPECT_BYTES(test, output, written, expected, sizeof(expected));

  written = 99U;
  TCPIP_EXPECT_U32(
      test, tcpip_l09_encode_name("www.example.com.", 16U, output, sizeof(output), &written),
      TCPIP_L09_OK);
  TCPIP_EXPECT_SIZE(test, written, sizeof(expected));
  TCPIP_EXPECT_BYTES(test, output, written, expected, sizeof(expected));

  written = 99U;
  TCPIP_EXPECT_U32(
      test, tcpip_l09_encode_name(".", 1U, output, sizeof(output), &written), TCPIP_L09_OK);
  TCPIP_EXPECT_SIZE(test, written, 1U);
  TCPIP_EXPECT_U32(test, output[0], 0U);

  memset(output, 0xa5, sizeof(output));
  written = 99U;
  TCPIP_EXPECT_U32(
      test, tcpip_l09_encode_name("example.com", 11U, output, 12U, &written),
      TCPIP_L09_CAPACITY);
  TCPIP_EXPECT_SIZE(test, written, 0U);
  TCPIP_EXPECT_U32(test, output[0], UINT8_C(0xa5));

  TCPIP_EXPECT_U32(
      test, tcpip_l09_encode_name("bad..name", 9U, output, sizeof(output), &written),
      TCPIP_L09_MALFORMED);
  {
    char long_label[64];
    memset(long_label, 'a', sizeof(long_label));
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_encode_name(long_label, sizeof(long_label), output, sizeof(output), &written),
        TCPIP_L09_MALFORMED);
  }
  {
    const char embedded_zero[] = {'a', '\0', 'b'};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_encode_name(
            embedded_zero, sizeof(embedded_zero), output, sizeof(output), &written),
        TCPIP_L09_MALFORMED);
  }
  TCPIP_EXPECT_U32(
      test, tcpip_l09_encode_name(NULL, 1U, output, sizeof(output), &written),
      TCPIP_L09_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, written, 0U);
  TCPIP_EXPECT_U32(
      test, tcpip_l09_encode_name("a", 1U, output, sizeof(output), NULL),
      TCPIP_L09_INVALID_ARGUMENT);
}

static void test_decode_name(tcpip_test_context *test) {
  uint8_t output[64];
  size_t next_offset = 99U;
  size_t written = 99U;
  tcpip_l09_status status;

  memset(output, 0xa5, sizeof(output));
  status = tcpip_l09_decode_name(
      tcpip_l09_response, sizeof(tcpip_l09_response), 12U, output, sizeof(output),
      &next_offset, &written);
  TCPIP_EXPECT_U32(test, status, TCPIP_L09_OK);
  if (status == TCPIP_L09_OK) {
    TCPIP_EXPECT_CSTR(test, (const char *)output, "example.com");
    TCPIP_EXPECT_SIZE(test, next_offset, 25U);
    TCPIP_EXPECT_SIZE(test, written, 11U);
  }

  status = tcpip_l09_decode_name(
      tcpip_l09_response, sizeof(tcpip_l09_response), 29U, output, sizeof(output),
      &next_offset, &written);
  TCPIP_EXPECT_U32(test, status, TCPIP_L09_OK);
  if (status == TCPIP_L09_OK) {
    TCPIP_EXPECT_CSTR(test, (const char *)output, "example.com");
    TCPIP_EXPECT_SIZE(test, next_offset, 31U);
    TCPIP_EXPECT_SIZE(test, written, 11U);
  }

  memset(output, 0xa5, sizeof(output));
  next_offset = 99U;
  written = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_decode_name(
          tcpip_l09_response, sizeof(tcpip_l09_response), 12U, output, 11U,
          &next_offset, &written),
      TCPIP_L09_CAPACITY);
  TCPIP_EXPECT_SIZE(test, next_offset, 0U);
  TCPIP_EXPECT_SIZE(test, written, 0U);
  TCPIP_EXPECT_U32(test, output[0], UINT8_C(0xa5));

  {
    static const uint8_t truncated_label[] = {3U, 'w', 'w'};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_decode_name(
            truncated_label, sizeof(truncated_label), 0U, output, sizeof(output),
            &next_offset, &written),
        TCPIP_L09_TRUNCATED);
  }
  {
    static const uint8_t truncated_pointer[] = {0xc0U};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_decode_name(
            truncated_pointer, sizeof(truncated_pointer), 0U, output, sizeof(output),
            &next_offset, &written),
        TCPIP_L09_TRUNCATED);
  }
  {
    static const uint8_t bad_pointer[] = {0xc0U, 0x10U};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_decode_name(
            bad_pointer, sizeof(bad_pointer), 0U, output, sizeof(output),
            &next_offset, &written),
        TCPIP_L09_MALFORMED);
  }
  {
    static const uint8_t pointer_cycle[] = {0xc0U, 0x00U};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_decode_name(
            pointer_cycle, sizeof(pointer_cycle), 0U, output, sizeof(output),
            &next_offset, &written),
        TCPIP_L09_MALFORMED);
  }
  {
    static const uint8_t two_pointer_cycle[] = {0xc0U, 0x02U, 0xc0U, 0x00U};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_decode_name(
            two_pointer_cycle, sizeof(two_pointer_cycle), 0U, output, sizeof(output),
            &next_offset, &written),
        TCPIP_L09_MALFORMED);
  }
  {
    static const uint8_t reserved_label[] = {0x40U, 0x00U};
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_decode_name(
            reserved_label, sizeof(reserved_label), 0U, output, sizeof(output),
            &next_offset, &written),
        TCPIP_L09_MALFORMED);
  }

  next_offset = 99U;
  written = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_decode_name(NULL, 1U, 0U, output, sizeof(output), &next_offset, &written),
      TCPIP_L09_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, next_offset, 0U);
  TCPIP_EXPECT_SIZE(test, written, 0U);
}

static void test_query_and_header(tcpip_test_context *test) {
  uint8_t output[64];
  size_t written = 99U;
  tcpip_l09_dns_header header;

  memset(output, 0xa5, sizeof(output));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_build_query(
          UINT16_C(0x1234), "example.com", 11U, UINT16_C(1), output, sizeof(output),
          &written),
      TCPIP_L09_OK);
  TCPIP_EXPECT_SIZE(test, written, sizeof(tcpip_l09_query));
  TCPIP_EXPECT_BYTES(test, output, written, tcpip_l09_query, sizeof(tcpip_l09_query));

  memset(output, 0xa5, sizeof(output));
  written = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_build_query(
          UINT16_C(0x1234), "example.com", 11U, UINT16_C(1), output,
          sizeof(tcpip_l09_query) - 1U, &written),
      TCPIP_L09_CAPACITY);
  TCPIP_EXPECT_SIZE(test, written, 0U);
  TCPIP_EXPECT_U32(test, output[0], UINT8_C(0xa5));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_build_query(1U, "a", 1U, 0U, output, sizeof(output), &written),
      TCPIP_L09_INVALID_ARGUMENT);

  memset(&header, 0xa5, sizeof(header));
  TCPIP_EXPECT_U32(
      test, tcpip_l09_parse_message(tcpip_l09_response, sizeof(tcpip_l09_response), &header),
      TCPIP_L09_OK);
  TCPIP_EXPECT_U16(test, header.id, UINT16_C(0xbeef));
  TCPIP_EXPECT_U16(test, header.flags, UINT16_C(0x8180));
  TCPIP_EXPECT_U16(test, header.question_count, 1U);
  TCPIP_EXPECT_U16(test, header.answer_count, 1U);
  TCPIP_EXPECT_U16(test, header.authority_count, 0U);
  TCPIP_EXPECT_U16(test, header.additional_count, 0U);
  TCPIP_EXPECT_U32(test, header.rcode, 0U);

  memset(&header, 0xa5, sizeof(header));
  TCPIP_EXPECT_U32(
      test, tcpip_l09_parse_message(tcpip_l09_nxdomain, sizeof(tcpip_l09_nxdomain), &header),
      TCPIP_L09_OK);
  TCPIP_EXPECT_U16(test, header.id, UINT16_C(0xbeef));
  TCPIP_EXPECT_U32(test, header.rcode, 3U);

  memset(&header, 0xa5, sizeof(header));
  TCPIP_EXPECT_U32(
      test, tcpip_l09_parse_message(tcpip_l09_response, 11U, &header),
      TCPIP_L09_TRUNCATED);
  TCPIP_EXPECT_U16(test, header.id, 0U);
  TCPIP_EXPECT_U16(test, header.flags, 0U);
  TCPIP_EXPECT_U16(test, header.question_count, 0U);
  TCPIP_EXPECT_U32(
      test, tcpip_l09_parse_message(NULL, 12U, &header), TCPIP_L09_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_parse_message(tcpip_l09_response, sizeof(tcpip_l09_response), NULL),
      TCPIP_L09_INVALID_ARGUMENT);
}

static void test_first_a(tcpip_test_context *test) {
  static const uint8_t expected_address[4] = {192U, 0U, 2U, 1U};
  uint8_t address[4] = {0xa5U, 0xa5U, 0xa5U, 0xa5U};
  uint32_t ttl = UINT32_MAX;

  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_first_a(tcpip_l09_response, sizeof(tcpip_l09_response), address, &ttl),
      TCPIP_L09_OK);
  TCPIP_EXPECT_BYTES(test, address, sizeof(address), expected_address, sizeof(expected_address));
  TCPIP_EXPECT_U32(test, ttl, UINT32_C(60));

  memset(address, 0xa5, sizeof(address));
  ttl = UINT32_MAX;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_first_a(tcpip_l09_nxdomain, sizeof(tcpip_l09_nxdomain), address, &ttl),
      TCPIP_L09_MALFORMED);
  {
    static const uint8_t zero_address[4] = {0U, 0U, 0U, 0U};
    TCPIP_EXPECT_BYTES(test, address, sizeof(address), zero_address, sizeof(zero_address));
  }
  TCPIP_EXPECT_U32(test, ttl, 0U);

  TCPIP_EXPECT_U32(
      test,
      tcpip_l09_first_a(tcpip_l09_response, sizeof(tcpip_l09_response) - 1U, address, &ttl),
      TCPIP_L09_TRUNCATED);
  {
    uint8_t truncated_response[sizeof(tcpip_l09_response)];
    memcpy(truncated_response, tcpip_l09_response, sizeof(truncated_response));
    truncated_response[2] |= UINT8_C(0x02);
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_first_a(truncated_response, sizeof(truncated_response), address, &ttl),
        TCPIP_L09_TRUNCATED);
  }
  {
    uint8_t query_as_response[sizeof(tcpip_l09_query)];
    memcpy(query_as_response, tcpip_l09_query, sizeof(query_as_response));
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_first_a(query_as_response, sizeof(query_as_response), address, &ttl),
        TCPIP_L09_MALFORMED);
  }
  {
    uint8_t reserved_z_response[sizeof(tcpip_l09_response)];
    memcpy(reserved_z_response, tcpip_l09_response, sizeof(reserved_z_response));
    reserved_z_response[3] |= UINT8_C(0x40);
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_first_a(reserved_z_response, sizeof(reserved_z_response), address, &ttl),
        TCPIP_L09_MALFORMED);
  }
  {
    uint8_t modern_flags_response[sizeof(tcpip_l09_response)];
    memcpy(modern_flags_response, tcpip_l09_response, sizeof(modern_flags_response));
    modern_flags_response[3] |= UINT8_C(0x30);
    TCPIP_EXPECT_U32(
        test,
        tcpip_l09_first_a(modern_flags_response, sizeof(modern_flags_response), address, &ttl),
        TCPIP_L09_OK);
  }
  TCPIP_EXPECT_U32(
      test, tcpip_l09_first_a(NULL, sizeof(tcpip_l09_response), address, &ttl),
      TCPIP_L09_INVALID_ARGUMENT);
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "lesson 09 DNS wire format");
  test_encode_name(&test);
  test_decode_name(&test);
  test_query_and_header(&test);
  test_first_a(&test);
  return tcpip_test_finish(&test);
}
