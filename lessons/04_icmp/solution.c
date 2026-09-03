#include "lesson.h"

#include <limits.h>
#include <string.h>

static uint16_t tcpip_l04_read_be16(const uint8_t *data, size_t offset) {
  return (uint16_t)(((uint16_t)data[offset] << 8U) | (uint16_t)data[offset + 1U]);
}

static void tcpip_l04_write_be16(uint8_t *data, size_t offset, uint16_t value) {
  data[offset] = (uint8_t)(value >> 8U);
  data[offset + 1U] = (uint8_t)(value & UINT16_C(0x00ff));
}

static uint16_t tcpip_l04_checksum(const uint8_t *data, size_t data_length) {
  uint32_t sum = 0U;
  size_t offset = 0U;

  while (data_length - offset >= 2U) {
    sum += (uint32_t)tcpip_l04_read_be16(data, offset);
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
    offset += 2U;
  }
  if (offset < data_length) {
    sum += (uint32_t)data[offset] << 8U;
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)~sum;
}

tcpip_l04_status tcpip_l04_parse_message(
    const uint8_t *message,
    size_t message_length,
    tcpip_l04_message *out_message) {
  tcpip_l04_message parsed;

  if (out_message != NULL) {
    memset(out_message, 0, sizeof(*out_message));
  }
  if (message == NULL || out_message == NULL) {
    return TCPIP_L04_INVALID_ARGUMENT;
  }
  if (message_length < TCPIP_L04_HEADER_LENGTH) {
    return TCPIP_L04_TRUNCATED;
  }
  if (tcpip_l04_checksum(message, message_length) != 0U) {
    return TCPIP_L04_MALFORMED;
  }

  memset(&parsed, 0, sizeof(parsed));
  parsed.type = message[0U];
  parsed.code = message[1U];
  parsed.checksum = tcpip_l04_read_be16(message, 2U);
  parsed.body_offset = TCPIP_L04_HEADER_LENGTH;
  parsed.body_length = message_length - TCPIP_L04_HEADER_LENGTH;
  *out_message = parsed;
  return TCPIP_L04_OK;
}

tcpip_l04_status tcpip_l04_build_echo(
    uint8_t type,
    uint16_t identifier,
    uint16_t sequence,
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_message_length) {
  size_t message_length;
  uint16_t checksum;

  if (out_message_length != NULL) {
    *out_message_length = 0U;
  }
  if ((payload == NULL && payload_length != 0U) || destination == NULL ||
      out_message_length == NULL) {
    return TCPIP_L04_INVALID_ARGUMENT;
  }
  if (type != TCPIP_L04_ECHO_REQUEST && type != TCPIP_L04_ECHO_REPLY) {
    return TCPIP_L04_MALFORMED;
  }
  if (payload_length > (size_t)UINT16_MAX - TCPIP_L04_ECHO_HEADER_LENGTH) {
    return TCPIP_L04_MALFORMED;
  }

  message_length = TCPIP_L04_ECHO_HEADER_LENGTH + payload_length;
  if (destination_capacity < message_length) {
    return TCPIP_L04_CAPACITY;
  }

  if (payload_length != 0U) {
    memmove(destination + TCPIP_L04_ECHO_HEADER_LENGTH, payload, payload_length);
  }
  destination[0U] = type;
  destination[1U] = 0U;
  destination[2U] = 0U;
  destination[3U] = 0U;
  tcpip_l04_write_be16(destination, 4U, identifier);
  tcpip_l04_write_be16(destination, 6U, sequence);
  checksum = tcpip_l04_checksum(destination, message_length);
  tcpip_l04_write_be16(destination, 2U, checksum);
  *out_message_length = message_length;
  return TCPIP_L04_OK;
}

tcpip_l04_status tcpip_l04_make_echo_reply(
    const uint8_t *request,
    size_t request_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_reply_length) {
  tcpip_l04_message parsed;
  tcpip_l04_status status;
  uint16_t identifier;
  uint16_t sequence;

  if (out_reply_length != NULL) {
    *out_reply_length = 0U;
  }
  if (request == NULL || destination == NULL || out_reply_length == NULL) {
    return TCPIP_L04_INVALID_ARGUMENT;
  }

  status = tcpip_l04_parse_message(request, request_length, &parsed);
  if (status != TCPIP_L04_OK) {
    return status;
  }
  if (parsed.type != TCPIP_L04_ECHO_REQUEST || parsed.code != 0U ||
      parsed.body_length < 4U) {
    return TCPIP_L04_MALFORMED;
  }
  if (destination_capacity < request_length) {
    return TCPIP_L04_CAPACITY;
  }

  identifier = tcpip_l04_read_be16(request, 4U);
  sequence = tcpip_l04_read_be16(request, 6U);
  return tcpip_l04_build_echo(
      TCPIP_L04_ECHO_REPLY,
      identifier,
      sequence,
      request + TCPIP_L04_ECHO_HEADER_LENGTH,
      request_length - TCPIP_L04_ECHO_HEADER_LENGTH,
      destination,
      destination_capacity,
      out_reply_length);
}
