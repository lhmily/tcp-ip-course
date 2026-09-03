#include "lesson.h"

#include <string.h>

tcpip_l01_status tcpip_l01_read_be16(
    const uint8_t *data, size_t data_len, size_t offset, uint16_t *out_value) {
  if (out_value != NULL) {
    *out_value = 0U;
  }
  if (out_value == NULL || data == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (offset > data_len || data_len - offset < 2U) {
    return TCPIP_L01_TRUNCATED;
  }

  /* TODO(lesson 01): combine two network-order octets into one host value. */
  return TCPIP_L01_TODO;
}

tcpip_l01_status tcpip_l01_write_be16(
    uint8_t *dst, size_t dst_len, size_t offset, uint16_t value) {
  (void)value;
  if (dst == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (offset > dst_len || dst_len - offset < 2U) {
    return TCPIP_L01_CAPACITY;
  }

  /* TODO(lesson 01): split the host value without partially writing dst. */
  return TCPIP_L01_TODO;
}

tcpip_l01_status tcpip_l01_parse_ipv4(
    const char *text, size_t text_len, uint8_t out_address[4]) {
  if (out_address != NULL) {
    memset(out_address, 0, 4U);
  }
  if (text == NULL || out_address == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (text_len == 0U) {
    return TCPIP_L01_MALFORMED;
  }

  /* TODO(lesson 01): parse exactly four bounded decimal components into a temporary. */
  return TCPIP_L01_TODO;
}

tcpip_l01_status tcpip_l01_prefix_contains(
    const uint8_t address[4],
    const uint8_t network[4],
    uint8_t prefix_length,
    bool *out_contains) {
  if (out_contains != NULL) {
    *out_contains = false;
  }
  if (address == NULL || network == NULL || out_contains == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (prefix_length > 32U) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }

  /* TODO(lesson 01): compare full prefix octets and the optional partial octet. */
  return TCPIP_L01_TODO;
}

tcpip_l01_status tcpip_l01_checksum16(
    const uint8_t *data, size_t data_len, uint16_t *out_checksum) {
  if (out_checksum != NULL) {
    *out_checksum = 0U;
  }
  if (out_checksum == NULL || (data == NULL && data_len != 0U)) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }

  /* TODO(lesson 01): add big-endian words, pad an odd byte, fold carries, and complement. */
  return TCPIP_L01_TODO;
}
