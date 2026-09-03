#include "lesson.h"

#include <string.h>

#define TCPIP_L09_DNS_HEADER_SIZE 12U
#define TCPIP_L09_MAX_WIRE_NAME 255U

static tcpip_l09_status tcpip_l09_validate_text_name(
    const char *name,
    size_t name_len,
    size_t *encoded_len) {
  size_t input_end = name_len;
  size_t label_start = 0U;
  size_t needed = 1U;
  size_t index;

  *encoded_len = 0U;
  if (name_len != 0U && name == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (name_len == 1U && name[0] == (uint8_t)'.') {
    input_end = 0U;
  } else if (name_len != 0U && name[name_len - 1U] == (uint8_t)'.') {
    input_end -= 1U;
  }
  for (index = 0U; index < input_end; index += 1U) {
    if (name[index] == 0U) {
      return TCPIP_L09_MALFORMED;
    }
    if (name[index] == (uint8_t)'.') {
      const size_t label_len = index - label_start;
      if (label_len == 0U || label_len > 63U) {
        return TCPIP_L09_MALFORMED;
      }
      if (needed > TCPIP_L09_MAX_WIRE_NAME - (label_len + 1U)) {
        return TCPIP_L09_MALFORMED;
      }
      needed += label_len + 1U;
      label_start = index + 1U;
    }
  }
  if (input_end != 0U) {
    const size_t label_len = input_end - label_start;
    if (label_len == 0U || label_len > 63U) {
      return TCPIP_L09_MALFORMED;
    }
    if (needed > TCPIP_L09_MAX_WIRE_NAME - (label_len + 1U)) {
      return TCPIP_L09_MALFORMED;
    }
    needed += label_len + 1U;
  }
  *encoded_len = needed;
  return TCPIP_L09_OK;
}

tcpip_l09_status tcpip_l09_encode_name(
    const char *name,
    size_t name_len,
    uint8_t *out,
    size_t out_capacity,
    size_t *written) {
  size_t encoded_len = 0U;
  tcpip_l09_status status;

  if (written == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  *written = 0U;
  if (out_capacity != 0U && out == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  status = tcpip_l09_validate_text_name(name, name_len, &encoded_len);
  if (status != TCPIP_L09_OK) {
    return status;
  }
  if (encoded_len > out_capacity) {
    return TCPIP_L09_CAPACITY;
  }
  return TCPIP_L09_TODO;
}

tcpip_l09_status tcpip_l09_decode_name(
    const uint8_t *message,
    size_t message_len,
    size_t offset,
    uint8_t *out,
    size_t out_capacity,
    size_t *next_offset,
    size_t *written) {
  if (next_offset == NULL || written == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  *next_offset = 0U;
  *written = 0U;
  if (message == NULL || (out_capacity != 0U && out == NULL)) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (offset >= message_len) {
    return TCPIP_L09_TRUNCATED;
  }
  return TCPIP_L09_TODO;
}

tcpip_l09_status tcpip_l09_build_query(
    uint16_t id,
    const char *name,
    size_t name_len,
    uint16_t qtype,
    uint8_t *out,
    size_t out_capacity,
    size_t *written) {
  size_t encoded_len = 0U;
  tcpip_l09_status status;

  (void)id;
  if (written == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  *written = 0U;
  if ((out_capacity != 0U && out == NULL) || qtype == 0U) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  status = tcpip_l09_validate_text_name(name, name_len, &encoded_len);
  if (status != TCPIP_L09_OK) {
    return status;
  }
  if (encoded_len > SIZE_MAX - (TCPIP_L09_DNS_HEADER_SIZE + 4U) ||
      TCPIP_L09_DNS_HEADER_SIZE + encoded_len + 4U > out_capacity) {
    return TCPIP_L09_CAPACITY;
  }
  return TCPIP_L09_TODO;
}

tcpip_l09_status tcpip_l09_parse_message(
    const uint8_t *message,
    size_t message_len,
    tcpip_l09_dns_header *header) {
  if (header == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  memset(header, 0, sizeof(*header));
  if (message == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (message_len < TCPIP_L09_DNS_HEADER_SIZE) {
    return TCPIP_L09_TRUNCATED;
  }
  return TCPIP_L09_TODO;
}

tcpip_l09_status tcpip_l09_first_a(
    const uint8_t *message,
    size_t message_len,
    uint8_t out[4],
    uint32_t *ttl) {
  if (out == NULL || ttl == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  memset(out, 0, 4U);
  *ttl = 0U;
  if (message == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (message_len < TCPIP_L09_DNS_HEADER_SIZE) {
    return TCPIP_L09_TRUNCATED;
  }
  return TCPIP_L09_TODO;
}
