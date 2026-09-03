#include "lesson.h"

#include <limits.h>
#include <string.h>

tcpip_l04_status tcpip_l04_parse_message(
    const uint8_t *message,
    size_t message_length,
    tcpip_l04_message *out_message) {
  if (out_message != NULL) {
    memset(out_message, 0, sizeof(*out_message));
  }
  if (message == NULL || out_message == NULL) {
    return TCPIP_L04_INVALID_ARGUMENT;
  }
  if (message_length < TCPIP_L04_HEADER_LENGTH) {
    return TCPIP_L04_TRUNCATED;
  }

  /* TODO(lesson 04): verify the complete checksum, then publish type, code, and body span. */
  return TCPIP_L04_TODO;
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

  (void)identifier;
  (void)sequence;
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

  /* TODO(lesson 04): write the echo fields and payload, then insert the checksum. */
  return TCPIP_L04_TODO;
}

tcpip_l04_status tcpip_l04_make_echo_reply(
    const uint8_t *request,
    size_t request_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_reply_length) {
  if (out_reply_length != NULL) {
    *out_reply_length = 0U;
  }
  if (request == NULL || destination == NULL || out_reply_length == NULL) {
    return TCPIP_L04_INVALID_ARGUMENT;
  }
  if (request_length < TCPIP_L04_HEADER_LENGTH) {
    return TCPIP_L04_TRUNCATED;
  }

  /* TODO(lesson 04): validate an echo request before checking reply capacity, then rebuild it atomically. */
  (void)destination_capacity;
  return TCPIP_L04_TODO;
}
