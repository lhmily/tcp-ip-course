#include "lesson.h"

#include <limits.h>
#include <stdint.h>

static int tcpip_l08_valid_buffer(const uint8_t *buffer, size_t length) {
  return buffer != NULL || length == 0U;
}

static int tcpip_l08_valid_mutable_buffer(uint8_t *buffer, size_t length) {
  return buffer != NULL || length == 0U;
}

tcpip_l08_status tcpip_l08_send_all(
    int fd, const uint8_t *data, size_t len, int timeout_ms) {
  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_buffer(data, len)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  return TCPIP_L08_TODO;
}

tcpip_l08_status tcpip_l08_recv_exact(
    int fd, uint8_t *out, size_t len, int timeout_ms) {
  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_mutable_buffer(out, len)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  return TCPIP_L08_TODO;
}

tcpip_l08_status tcpip_l08_send_frame(
    int fd, const uint8_t *data, size_t len, int timeout_ms) {
  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_buffer(data, len)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  if (len > (size_t)UINT32_MAX) {
    return TCPIP_L08_MALFORMED;
  }
  return TCPIP_L08_TODO;
}

tcpip_l08_status tcpip_l08_recv_frame(
    int fd, uint8_t *out, size_t cap, size_t *out_len, int timeout_ms) {
  if (out_len == NULL) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  *out_len = 0U;
  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_mutable_buffer(out, cap)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  return TCPIP_L08_TODO;
}

tcpip_l08_status tcpip_l08_serve_one(int listen_fd, int timeout_ms) {
  if (listen_fd < 0 || timeout_ms < 0) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  return TCPIP_L08_TODO;
}
