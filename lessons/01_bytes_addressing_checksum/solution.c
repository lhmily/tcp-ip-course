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

  *out_value = (uint16_t)(((uint16_t)data[offset] << 8U) | (uint16_t)data[offset + 1U]);
  return TCPIP_L01_OK;
}

tcpip_l01_status tcpip_l01_write_be16(
    uint8_t *dst, size_t dst_len, size_t offset, uint16_t value) {
  if (dst == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (offset > dst_len || dst_len - offset < 2U) {
    return TCPIP_L01_CAPACITY;
  }

  dst[offset] = (uint8_t)(value >> 8U);
  dst[offset + 1U] = (uint8_t)(value & UINT16_C(0x00ff));
  return TCPIP_L01_OK;
}

tcpip_l01_status tcpip_l01_parse_ipv4(
    const char *text, size_t text_len, uint8_t out_address[4]) {
  uint8_t parsed[4] = {0U, 0U, 0U, 0U};
  size_t position = 0U;
  size_t part = 0U;

  if (out_address != NULL) {
    memset(out_address, 0, 4U);
  }
  if (text == NULL || out_address == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (text_len == 0U) {
    return TCPIP_L01_MALFORMED;
  }

  for (part = 0U; part < 4U; ++part) {
    unsigned value = 0U;
    size_t digits = 0U;

    while (position < text_len && text[position] >= '0' && text[position] <= '9') {
      if (digits == 3U) {
        return TCPIP_L01_MALFORMED;
      }
      value = value * 10U + (unsigned)(text[position] - '0');
      if (value > 255U) {
        return TCPIP_L01_MALFORMED;
      }
      ++digits;
      ++position;
    }
    if (digits == 0U) {
      return TCPIP_L01_MALFORMED;
    }
    parsed[part] = (uint8_t)value;

    if (part < 3U) {
      if (position >= text_len || text[position] != '.') {
        return TCPIP_L01_MALFORMED;
      }
      ++position;
    }
  }

  if (position != text_len) {
    return TCPIP_L01_MALFORMED;
  }

  memcpy(out_address, parsed, sizeof(parsed));
  return TCPIP_L01_OK;
}

tcpip_l01_status tcpip_l01_prefix_contains(
    const uint8_t address[4],
    const uint8_t network[4],
    uint8_t prefix_length,
    bool *out_contains) {
  size_t full_bytes = 0U;
  unsigned remaining_bits = 0U;
  size_t index = 0U;

  if (out_contains != NULL) {
    *out_contains = false;
  }
  if (address == NULL || network == NULL || out_contains == NULL) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }
  if (prefix_length > 32U) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }

  full_bytes = (size_t)(prefix_length / 8U);
  remaining_bits = (unsigned)(prefix_length % 8U);
  for (index = 0U; index < full_bytes; ++index) {
    if (address[index] != network[index]) {
      return TCPIP_L01_OK;
    }
  }
  if (remaining_bits != 0U) {
    uint8_t mask = (uint8_t)(UINT8_C(0xff) << (8U - remaining_bits));
    if ((address[full_bytes] & mask) != (network[full_bytes] & mask)) {
      return TCPIP_L01_OK;
    }
  }

  *out_contains = true;
  return TCPIP_L01_OK;
}

tcpip_l01_status tcpip_l01_checksum16(
    const uint8_t *data, size_t data_len, uint16_t *out_checksum) {
  uint32_t sum = 0U;
  size_t offset = 0U;

  if (out_checksum != NULL) {
    *out_checksum = 0U;
  }
  if (out_checksum == NULL || (data == NULL && data_len != 0U)) {
    return TCPIP_L01_INVALID_ARGUMENT;
  }

  while (data_len - offset >= 2U) {
    uint16_t word = (uint16_t)(((uint16_t)data[offset] << 8U) | data[offset + 1U]);
    sum += word;
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
    offset += 2U;
  }
  if (offset < data_len) {
    sum += (uint32_t)data[offset] << 8U;
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);

  *out_checksum = (uint16_t)(~sum & UINT32_C(0xffff));
  return TCPIP_L01_OK;
}
