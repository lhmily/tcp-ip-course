#include "lesson.h"

#include <string.h>

static int tcpip_l10_valid_bytes(const uint8_t *data, size_t length) {
  return data != NULL || length == 0u;
}

tcpip_l10_status tcpip_l10_build_request(
    const uint8_t *path,
    size_t path_len,
    const uint8_t *body,
    size_t body_len,
    uint8_t *out,
    size_t cap,
    size_t *written) {
  if (written == NULL) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  *written = 0u;
  if (path == NULL || path_len == 0u || !tcpip_l10_valid_bytes(body, body_len) ||
      out == NULL || cap == 0u) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  if (body_len > TCPIP_L10_MAX_BODY_BYTES) {
    return TCPIP_L10_CAPACITY;
  }
  return TCPIP_L10_TODO;
}

tcpip_l10_status tcpip_l10_parse_message(
    const uint8_t *input, size_t input_len, tcpip_l10_message_view *message) {
  if (message == NULL) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  memset(message, 0, sizeof(*message));
  if (!tcpip_l10_valid_bytes(input, input_len)) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  return TCPIP_L10_TODO;
}

tcpip_l10_status tcpip_l10_send_message(
    int fd,
    const uint8_t *message,
    size_t message_len,
    int timeout_ms,
    size_t *sent) {
  if (sent == NULL) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  *sent = 0u;
  if (fd < 0 || !tcpip_l10_valid_bytes(message, message_len) || timeout_ms < 0) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  return TCPIP_L10_TODO;
}

tcpip_l10_status tcpip_l10_recv_message(
    int fd,
    uint8_t *out,
    size_t cap,
    int timeout_ms,
    size_t *received,
    tcpip_l10_message_view *message) {
  if (received != NULL) {
    *received = 0u;
  }
  if (message != NULL) {
    memset(message, 0, sizeof(*message));
  }
  if (received == NULL || message == NULL) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  if (fd < 0 || out == NULL || cap == 0u || timeout_ms < 0) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  return TCPIP_L10_TODO;
}

tcpip_l10_status tcpip_l10_serve_one(
    int fd,
    uint8_t *request_buffer,
    size_t request_capacity,
    int timeout_ms) {
  if (fd < 0 || request_buffer == NULL || request_capacity == 0u || timeout_ms < 0) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  return TCPIP_L10_TODO;
}
