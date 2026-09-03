#define _POSIX_C_SOURCE 200809L

#include "lesson.h"

#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TCPIP_L08_NS_PER_MS INT64_C(1000000)
#define TCPIP_L08_NS_PER_S INT64_C(1000000000)

static int tcpip_l08_valid_buffer(const uint8_t *buffer, size_t length) {
  return buffer != NULL || length == 0U;
}

static int tcpip_l08_valid_mutable_buffer(uint8_t *buffer, size_t length) {
  return buffer != NULL || length == 0U;
}

static tcpip_l08_status tcpip_l08_make_deadline(int timeout_ms, struct timespec *deadline) {
  int64_t nanoseconds;

  if (deadline == NULL || timeout_ms < 0) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) {
    return TCPIP_L08_SYSTEM;
  }
  nanoseconds = (int64_t)deadline->tv_nsec
      + (int64_t)timeout_ms * TCPIP_L08_NS_PER_MS;
  deadline->tv_sec += (time_t)(nanoseconds / TCPIP_L08_NS_PER_S);
  deadline->tv_nsec = (long)(nanoseconds % TCPIP_L08_NS_PER_S);
  return TCPIP_L08_OK;
}

static tcpip_l08_status tcpip_l08_remaining_ms(
    const struct timespec *deadline, int *remaining_ms) {
  struct timespec now;
  int64_t seconds;
  int64_t nanoseconds;
  int64_t milliseconds;

  if (deadline == NULL || remaining_ms == NULL) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
    return TCPIP_L08_SYSTEM;
  }

  seconds = (int64_t)deadline->tv_sec - (int64_t)now.tv_sec;
  nanoseconds = (int64_t)deadline->tv_nsec - (int64_t)now.tv_nsec;
  if (nanoseconds < 0) {
    seconds -= 1;
    nanoseconds += TCPIP_L08_NS_PER_S;
  }
  if (seconds < 0 || (seconds == 0 && nanoseconds <= 0)) {
    *remaining_ms = 0;
    return TCPIP_L08_OK;
  }
  if (seconds > (int64_t)INT_MAX / INT64_C(1000)) {
    *remaining_ms = INT_MAX;
    return TCPIP_L08_OK;
  }
  milliseconds = seconds * INT64_C(1000)
      + (nanoseconds + TCPIP_L08_NS_PER_MS - 1) / TCPIP_L08_NS_PER_MS;
  if (milliseconds > (int64_t)INT_MAX) {
    *remaining_ms = INT_MAX;
  } else {
    *remaining_ms = (int)milliseconds;
  }
  return TCPIP_L08_OK;
}

static tcpip_l08_status tcpip_l08_wait(
    int fd, short events, const struct timespec *deadline) {
  struct pollfd descriptor;

  descriptor.fd = fd;
  descriptor.events = events;
  descriptor.revents = 0;
  for (;;) {
    int remaining_ms;
    int result;
    tcpip_l08_status status = tcpip_l08_remaining_ms(deadline, &remaining_ms);

    if (status != TCPIP_L08_OK) {
      return status;
    }
    descriptor.revents = 0;
    result = poll(&descriptor, 1U, remaining_ms);
    if (result > 0) {
      if ((descriptor.revents & POLLNVAL) != 0) {
        return TCPIP_L08_SYSTEM;
      }
      if ((descriptor.revents & (events | POLLERR | POLLHUP)) != 0) {
        return TCPIP_L08_OK;
      }
      continue;
    }
    if (result == 0) {
      return TCPIP_L08_TIMEOUT;
    }
    if (errno != EINTR) {
      return TCPIP_L08_SYSTEM;
    }
  }
}

static tcpip_l08_status tcpip_l08_prepare_send(int fd) {
#if defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL)
  const int enabled = 1;

  if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0) {
    return TCPIP_L08_SYSTEM;
  }
#else
  (void)fd;
#endif
  return TCPIP_L08_OK;
}

static tcpip_l08_status tcpip_l08_send_all_until(
    int fd, const uint8_t *data, size_t len, const struct timespec *deadline) {
  size_t offset = 0U;
  tcpip_l08_status status = tcpip_l08_prepare_send(fd);

  if (status != TCPIP_L08_OK) {
    return status;
  }
  while (offset < len) {
    int flags = 0;
    ssize_t sent;
    tcpip_l08_status wait_status = tcpip_l08_wait(fd, POLLOUT, deadline);

    if (wait_status != TCPIP_L08_OK) {
      return wait_status;
    }
#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif
#ifdef MSG_DONTWAIT
    flags |= MSG_DONTWAIT;
#endif
    sent = send(fd, data + offset, len - offset, flags);
    if (sent > 0) {
      offset += (size_t)sent;
      continue;
    }
    if (sent == 0) {
      return offset == 0U ? TCPIP_L08_EOF : TCPIP_L08_TRUNCATED;
    }
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    return TCPIP_L08_SYSTEM;
  }
  return TCPIP_L08_OK;
}

static tcpip_l08_status tcpip_l08_recv_exact_until(
    int fd, uint8_t *out, size_t len, const struct timespec *deadline) {
  size_t offset = 0U;

  while (offset < len) {
    ssize_t received;
    tcpip_l08_status status = tcpip_l08_wait(fd, POLLIN, deadline);

    if (status != TCPIP_L08_OK) {
      return status;
    }
#ifdef MSG_DONTWAIT
    received = recv(fd, out + offset, len - offset, MSG_DONTWAIT);
#else
    received = recv(fd, out + offset, len - offset, 0);
#endif
    if (received > 0) {
      offset += (size_t)received;
      continue;
    }
    if (received == 0) {
      return offset == 0U ? TCPIP_L08_EOF : TCPIP_L08_TRUNCATED;
    }
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    return TCPIP_L08_SYSTEM;
  }
  return TCPIP_L08_OK;
}

static tcpip_l08_status tcpip_l08_send_frame_until(
    int fd, const uint8_t *data, size_t len, const struct timespec *deadline) {
  uint32_t wire_length = (uint32_t)len;
  uint8_t header[4];
  tcpip_l08_status status;

  header[0] = (uint8_t)(wire_length >> 24U);
  header[1] = (uint8_t)(wire_length >> 16U);
  header[2] = (uint8_t)(wire_length >> 8U);
  header[3] = (uint8_t)wire_length;
  status = tcpip_l08_send_all_until(fd, header, sizeof(header), deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  return tcpip_l08_send_all_until(fd, data, len, deadline);
}

static tcpip_l08_status tcpip_l08_recv_frame_until(
    int fd,
    uint8_t *out,
    size_t cap,
    size_t *out_len,
    const struct timespec *deadline) {
  uint8_t header[4];
  uint32_t wire_length;
  size_t length;
  tcpip_l08_status status;

  status = tcpip_l08_recv_exact_until(fd, header, sizeof(header), deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  wire_length = ((uint32_t)header[0] << 24U)
      | ((uint32_t)header[1] << 16U)
      | ((uint32_t)header[2] << 8U)
      | (uint32_t)header[3];
#if SIZE_MAX < UINT32_MAX
  if (wire_length > (uint32_t)SIZE_MAX) {
    return TCPIP_L08_MALFORMED;
  }
#endif
  length = (size_t)wire_length;
  if (length > cap) {
    return TCPIP_L08_CAPACITY;
  }
  status = tcpip_l08_recv_exact_until(fd, out, length, deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  *out_len = length;
  return TCPIP_L08_OK;
}

tcpip_l08_status tcpip_l08_send_all(
    int fd, const uint8_t *data, size_t len, int timeout_ms) {
  struct timespec deadline;
  tcpip_l08_status status;

  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_buffer(data, len)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  status = tcpip_l08_make_deadline(timeout_ms, &deadline);
  if (status != TCPIP_L08_OK || len == 0U) {
    return status;
  }
  return tcpip_l08_send_all_until(fd, data, len, &deadline);
}

tcpip_l08_status tcpip_l08_recv_exact(
    int fd, uint8_t *out, size_t len, int timeout_ms) {
  struct timespec deadline;
  tcpip_l08_status status;

  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_mutable_buffer(out, len)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  status = tcpip_l08_make_deadline(timeout_ms, &deadline);
  if (status != TCPIP_L08_OK || len == 0U) {
    return status;
  }
  return tcpip_l08_recv_exact_until(fd, out, len, &deadline);
}

tcpip_l08_status tcpip_l08_send_frame(
    int fd, const uint8_t *data, size_t len, int timeout_ms) {
  struct timespec deadline;
  tcpip_l08_status status;

  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_buffer(data, len)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  if (len > (size_t)UINT32_MAX) {
    return TCPIP_L08_MALFORMED;
  }
  status = tcpip_l08_make_deadline(timeout_ms, &deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  return tcpip_l08_send_frame_until(fd, data, len, &deadline);
}

tcpip_l08_status tcpip_l08_recv_frame(
    int fd, uint8_t *out, size_t cap, size_t *out_len, int timeout_ms) {
  struct timespec deadline;
  tcpip_l08_status status;

  if (out_len == NULL) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  *out_len = 0U;
  if (fd < 0 || timeout_ms < 0 || !tcpip_l08_valid_mutable_buffer(out, cap)) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  status = tcpip_l08_make_deadline(timeout_ms, &deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  return tcpip_l08_recv_frame_until(fd, out, cap, out_len, &deadline);
}

tcpip_l08_status tcpip_l08_serve_one(int listen_fd, int timeout_ms) {
  struct timespec deadline;
  uint8_t payload[TCPIP_L08_SERVER_FRAME_CAPACITY];
  size_t payload_len = 0U;
  int client_fd;
  tcpip_l08_status status;

  if (listen_fd < 0 || timeout_ms < 0) {
    return TCPIP_L08_INVALID_ARGUMENT;
  }
  status = tcpip_l08_make_deadline(timeout_ms, &deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  status = tcpip_l08_wait(listen_fd, POLLIN, &deadline);
  if (status != TCPIP_L08_OK) {
    return status;
  }
  for (;;) {
    client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd >= 0) {
      break;
    }
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      status = tcpip_l08_wait(listen_fd, POLLIN, &deadline);
      if (status != TCPIP_L08_OK) {
        return status;
      }
      continue;
    }
    return TCPIP_L08_SYSTEM;
  }

  status = tcpip_l08_recv_frame_until(
      client_fd, payload, sizeof(payload), &payload_len, &deadline);
  if (status == TCPIP_L08_OK) {
    status = tcpip_l08_send_frame_until(client_fd, payload, payload_len, &deadline);
  }
  if (close(client_fd) != 0 && status == TCPIP_L08_OK) {
    status = TCPIP_L08_SYSTEM;
  }
  return status;
}
