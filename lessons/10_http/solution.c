#define _POSIX_C_SOURCE 200809L

#include "lesson.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int tcpip_l10_is_tchar(uint8_t byte) {
  if ((byte >= (uint8_t)'0' && byte <= (uint8_t)'9') ||
      (byte >= (uint8_t)'A' && byte <= (uint8_t)'Z') ||
      (byte >= (uint8_t)'a' && byte <= (uint8_t)'z')) {
    return 1;
  }
  return byte == (uint8_t)'!' || byte == (uint8_t)'#' || byte == (uint8_t)'$' ||
         byte == (uint8_t)'%' || byte == (uint8_t)'&' || byte == (uint8_t)'\'' ||
         byte == (uint8_t)'*' || byte == (uint8_t)'+' || byte == (uint8_t)'-' ||
         byte == (uint8_t)'.' || byte == (uint8_t)'^' || byte == (uint8_t)'_' ||
         byte == (uint8_t)'`' || byte == (uint8_t)'|' || byte == (uint8_t)'~';
}

static int tcpip_l10_ascii_equal(
    const uint8_t *left, size_t left_len, const char *right, size_t right_len) {
  if (left_len != right_len) {
    return 0;
  }
  for (size_t index = 0; index < left_len; index += 1u) {
    uint8_t byte = left[index];
    if (byte >= (uint8_t)'A' && byte <= (uint8_t)'Z') {
      byte = (uint8_t)(byte + ((uint8_t)'a' - (uint8_t)'A'));
    }
    if (byte != (uint8_t)right[index]) {
      return 0;
    }
  }
  return 1;
}

static int tcpip_l10_find_crlf(
    const uint8_t *input, size_t begin, size_t end, size_t *line_end) {
  for (size_t index = begin; index + 1u < end; index += 1u) {
    if (input[index] == (uint8_t)'\r' && input[index + 1u] == (uint8_t)'\n') {
      *line_end = index;
      return 1;
    }
    if (input[index] == (uint8_t)'\r' || input[index] == (uint8_t)'\n') {
      return -1;
    }
  }
  return 0;
}

static int tcpip_l10_valid_version(const uint8_t *data, size_t length) {
  return length == 8u && memcmp(data, "HTTP/1.1", 8u) == 0;
}

static tcpip_l10_status tcpip_l10_parse_start_line(
    const uint8_t *line, size_t length, tcpip_l10_message_view *message) {
  if (length >= 5u && memcmp(line, "HTTP/", 5u) == 0) {
    size_t first_space = 0u;
    while (first_space < length && line[first_space] != (uint8_t)' ') {
      first_space += 1u;
    }
    if (!tcpip_l10_valid_version(line, first_space) || first_space + 4u >= length) {
      return TCPIP_L10_MALFORMED;
    }
    const size_t code_at = first_space + 1u;
    if (line[code_at] < (uint8_t)'1' || line[code_at] > (uint8_t)'9' ||
        line[code_at + 1u] < (uint8_t)'0' || line[code_at + 1u] > (uint8_t)'9' ||
        line[code_at + 2u] < (uint8_t)'0' || line[code_at + 2u] > (uint8_t)'9') {
      return TCPIP_L10_MALFORMED;
    }
    if (code_at + 3u < length && line[code_at + 3u] != (uint8_t)' ') {
      return TCPIP_L10_MALFORMED;
    }
    for (size_t index = code_at + 4u; index < length; index += 1u) {
      if (line[index] < (uint8_t)' ' || line[index] == (uint8_t)127) {
        return TCPIP_L10_MALFORMED;
      }
    }
    message->kind = TCPIP_L10_MESSAGE_RESPONSE;
    message->version.data = line;
    message->version.length = first_space;
    message->status_code =
        (unsigned)(line[code_at] - (uint8_t)'0') * 100u +
        (unsigned)(line[code_at + 1u] - (uint8_t)'0') * 10u +
        (unsigned)(line[code_at + 2u] - (uint8_t)'0');
    if (code_at + 3u < length) {
      message->reason.data = line + code_at + 4u;
      message->reason.length = length - (code_at + 4u);
    } else {
      message->reason.data = line + length;
    }
    return TCPIP_L10_OK;
  }

  size_t first_space = 0u;
  while (first_space < length && line[first_space] != (uint8_t)' ') {
    if (!tcpip_l10_is_tchar(line[first_space])) {
      return TCPIP_L10_MALFORMED;
    }
    first_space += 1u;
  }
  if (first_space == 0u || first_space == length) {
    return TCPIP_L10_MALFORMED;
  }
  size_t second_space = first_space + 1u;
  while (second_space < length && line[second_space] != (uint8_t)' ') {
    if (line[second_space] <= (uint8_t)' ' || line[second_space] == (uint8_t)127) {
      return TCPIP_L10_MALFORMED;
    }
    second_space += 1u;
  }
  if (second_space == first_space + 1u || second_space == length ||
      !tcpip_l10_valid_version(line + second_space + 1u, length - second_space - 1u)) {
    return TCPIP_L10_MALFORMED;
  }

  message->kind = TCPIP_L10_MESSAGE_REQUEST;
  message->method.data = line;
  message->method.length = first_space;
  message->target.data = line + first_space + 1u;
  message->target.length = second_space - first_space - 1u;
  message->version.data = line + second_space + 1u;
  message->version.length = length - second_space - 1u;
  return TCPIP_L10_OK;
}

static tcpip_l10_status tcpip_l10_parse_content_length(
    const uint8_t *value, size_t length, size_t *result) {
  if (length == 0u) {
    return TCPIP_L10_MALFORMED;
  }
  size_t parsed = 0u;
  for (size_t index = 0u; index < length; index += 1u) {
    if (value[index] < (uint8_t)'0' || value[index] > (uint8_t)'9') {
      return TCPIP_L10_MALFORMED;
    }
    const size_t digit = (size_t)(value[index] - (uint8_t)'0');
    if (parsed > (SIZE_MAX - digit) / 10u) {
      return TCPIP_L10_MALFORMED;
    }
    parsed = parsed * 10u + digit;
  }
  if (parsed > TCPIP_L10_MAX_BODY_BYTES) {
    return TCPIP_L10_CAPACITY;
  }
  *result = parsed;
  return TCPIP_L10_OK;
}

tcpip_l10_status tcpip_l10_parse_message(
    const uint8_t *input, size_t input_len, tcpip_l10_message_view *message) {
  if (message == NULL) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  memset(message, 0, sizeof(*message));
  if (input == NULL && input_len != 0u) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  if (input_len == 0u) {
    return TCPIP_L10_TRUNCATED;
  }

  size_t header_end = 0u;
  int found_end = 0;
  const size_t scan_limit =
      input_len < TCPIP_L10_MAX_HEADER_BYTES ? input_len : TCPIP_L10_MAX_HEADER_BYTES;
  for (size_t index = 0u; index + 3u < scan_limit; index += 1u) {
    if (input[index] == (uint8_t)'\r' && input[index + 1u] == (uint8_t)'\n' &&
        input[index + 2u] == (uint8_t)'\r' && input[index + 3u] == (uint8_t)'\n') {
      header_end = index + 4u;
      found_end = 1;
      break;
    }
  }
  if (!found_end) {
    for (size_t index = 0u; index < scan_limit; index += 1u) {
      if (input[index] == (uint8_t)'\n' &&
          (index == 0u || input[index - 1u] != (uint8_t)'\r')) {
        return TCPIP_L10_MALFORMED;
      }
      if (input[index] == (uint8_t)'\r' && index + 1u < scan_limit &&
          input[index + 1u] != (uint8_t)'\n') {
        return TCPIP_L10_MALFORMED;
      }
    }
    return input_len >= TCPIP_L10_MAX_HEADER_BYTES ? TCPIP_L10_CAPACITY : TCPIP_L10_TRUNCATED;
  }

  size_t line_end = 0u;
  const int start_result = tcpip_l10_find_crlf(input, 0u, header_end, &line_end);
  if (start_result <= 0 || line_end == 0u) {
    return TCPIP_L10_MALFORMED;
  }
  tcpip_l10_status status = tcpip_l10_parse_start_line(input, line_end, message);
  if (status != TCPIP_L10_OK) {
    memset(message, 0, sizeof(*message));
    return status;
  }

  size_t cursor = line_end + 2u;
  int have_content_length = 0;
  size_t content_length = 0u;
  while (cursor + 1u < header_end) {
    if (input[cursor] == (uint8_t)'\r' && input[cursor + 1u] == (uint8_t)'\n') {
      cursor += 2u;
      break;
    }
    if (input[cursor] == (uint8_t)' ' || input[cursor] == (uint8_t)'\t') {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_MALFORMED;
    }
    if (message->header_count == TCPIP_L10_MAX_HEADERS) {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_CAPACITY;
    }
    const int line_result = tcpip_l10_find_crlf(input, cursor, header_end, &line_end);
    if (line_result <= 0 || line_end == cursor) {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_MALFORMED;
    }

    size_t colon = cursor;
    while (colon < line_end && input[colon] != (uint8_t)':') {
      if (!tcpip_l10_is_tchar(input[colon])) {
        memset(message, 0, sizeof(*message));
        return TCPIP_L10_MALFORMED;
      }
      colon += 1u;
    }
    if (colon == cursor || colon == line_end) {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_MALFORMED;
    }
    size_t value_begin = colon + 1u;
    while (value_begin < line_end &&
           (input[value_begin] == (uint8_t)' ' || input[value_begin] == (uint8_t)'\t')) {
      value_begin += 1u;
    }
    size_t value_end = line_end;
    while (value_end > value_begin &&
           (input[value_end - 1u] == (uint8_t)' ' || input[value_end - 1u] == (uint8_t)'\t')) {
      value_end -= 1u;
    }
    for (size_t index = value_begin; index < value_end; index += 1u) {
      if ((input[index] < (uint8_t)' ' && input[index] != (uint8_t)'\t') ||
          input[index] == (uint8_t)127) {
        memset(message, 0, sizeof(*message));
        return TCPIP_L10_MALFORMED;
      }
    }

    tcpip_l10_header_view *header = &message->headers[message->header_count];
    header->name.data = input + cursor;
    header->name.length = colon - cursor;
    header->value.data = input + value_begin;
    header->value.length = value_end - value_begin;
    message->header_count += 1u;

    if (tcpip_l10_ascii_equal(header->name.data, header->name.length, "transfer-encoding", 17u)) {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_MALFORMED;
    }
    if (tcpip_l10_ascii_equal(header->name.data, header->name.length, "content-length", 14u)) {
      size_t parsed_length = 0u;
      status = tcpip_l10_parse_content_length(
          header->value.data, header->value.length, &parsed_length);
      if (status != TCPIP_L10_OK ||
          (have_content_length && parsed_length != content_length)) {
        memset(message, 0, sizeof(*message));
        return status == TCPIP_L10_OK ? TCPIP_L10_MALFORMED : status;
      }
      have_content_length = 1;
      content_length = parsed_length;
    }
    cursor = line_end + 2u;
  }
  if (cursor != header_end) {
    memset(message, 0, sizeof(*message));
    return TCPIP_L10_MALFORMED;
  }
  if (content_length > SIZE_MAX - header_end) {
    memset(message, 0, sizeof(*message));
    return TCPIP_L10_CAPACITY;
  }
  const size_t message_length = header_end + content_length;
  if (input_len < message_length) {
    memset(message, 0, sizeof(*message));
    return TCPIP_L10_TRUNCATED;
  }
  message->content_length = content_length;
  message->body.data = input + header_end;
  message->body.length = content_length;
  message->message_length = message_length;
  return TCPIP_L10_OK;
}

static size_t tcpip_l10_decimal_size(size_t value) {
  size_t digits = 1u;
  while (value >= 10u) {
    value /= 10u;
    digits += 1u;
  }
  return digits;
}

static void tcpip_l10_write_decimal(uint8_t *out, size_t digits, size_t value) {
  for (size_t index = digits; index > 0u; index -= 1u) {
    out[index - 1u] = (uint8_t)('0' + (value % 10u));
    value /= 10u;
  }
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
  if (path == NULL || path_len == 0u || out == NULL ||
      (body == NULL && body_len != 0u)) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  if (body_len > TCPIP_L10_MAX_BODY_BYTES) {
    return TCPIP_L10_CAPACITY;
  }

  static const uint8_t prefix[] = "POST ";
  static const uint8_t middle[] =
      " HTTP/1.1\r\nHost: localhost\r\nContent-Length: ";
  static const uint8_t suffix[] = "\r\nConnection: close\r\n\r\n";
  const size_t digits = tcpip_l10_decimal_size(body_len);
  const size_t fixed = (sizeof(prefix) - 1u) + (sizeof(middle) - 1u) +
                       digits + (sizeof(suffix) - 1u);
  if (path_len > SIZE_MAX - fixed) {
    return TCPIP_L10_CAPACITY;
  }
  const size_t header_length = fixed + path_len;
  if (header_length > TCPIP_L10_MAX_HEADER_BYTES ||
      body_len > SIZE_MAX - header_length || header_length + body_len > cap) {
    return TCPIP_L10_CAPACITY;
  }
  if (path[0] != (uint8_t)'/') {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  for (size_t index = 0u; index < path_len; index += 1u) {
    if (path[index] <= (uint8_t)' ' || path[index] == (uint8_t)127) {
      return TCPIP_L10_INVALID_ARGUMENT;
    }
  }
  size_t cursor = 0u;
  memcpy(out + cursor, prefix, sizeof(prefix) - 1u);
  cursor += sizeof(prefix) - 1u;
  memcpy(out + cursor, path, path_len);
  cursor += path_len;
  memcpy(out + cursor, middle, sizeof(middle) - 1u);
  cursor += sizeof(middle) - 1u;
  tcpip_l10_write_decimal(out + cursor, digits, body_len);
  cursor += digits;
  memcpy(out + cursor, suffix, sizeof(suffix) - 1u);
  cursor += sizeof(suffix) - 1u;
  if (body_len != 0u) {
    memcpy(out + cursor, body, body_len);
  }
  cursor += body_len;
  *written = cursor;
  return TCPIP_L10_OK;
}

static int tcpip_l10_now(struct timespec *now) {
  return clock_gettime(CLOCK_MONOTONIC, now);
}

static struct timespec tcpip_l10_deadline_after(struct timespec now, int timeout_ms) {
  now.tv_sec += (time_t)(timeout_ms / 1000);
  now.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
  if (now.tv_nsec >= 1000000000L) {
    now.tv_sec += 1;
    now.tv_nsec -= 1000000000L;
  }
  return now;
}

static int tcpip_l10_remaining_ms(const struct timespec *deadline) {
  struct timespec now;
  if (tcpip_l10_now(&now) != 0) {
    return -1;
  }
  time_t seconds = deadline->tv_sec - now.tv_sec;
  long nanoseconds = deadline->tv_nsec - now.tv_nsec;
  if (nanoseconds < 0L) {
    seconds -= 1;
    nanoseconds += 1000000000L;
  }
  if (seconds < 0 || (seconds == 0 && nanoseconds <= 0L)) {
    return 0;
  }
  if (seconds > (time_t)(INT_MAX / 1000)) {
    return INT_MAX;
  }
  const long long milliseconds =
      (long long)seconds * 1000LL + ((long long)nanoseconds + 999999LL) / 1000000LL;
  return milliseconds > (long long)INT_MAX ? INT_MAX : (int)milliseconds;
}

static tcpip_l10_status tcpip_l10_wait_fd(
    int fd, short events, const struct timespec *deadline) {
  for (;;) {
    const int remaining = tcpip_l10_remaining_ms(deadline);
    if (remaining < 0) {
      return TCPIP_L10_SYSTEM;
    }
    if (remaining == 0) {
      return TCPIP_L10_TIMEOUT;
    }
    struct pollfd descriptor;
    descriptor.fd = fd;
    descriptor.events = events;
    descriptor.revents = 0;
    const int result = poll(&descriptor, 1, remaining);
    if (result > 0) {
      if ((descriptor.revents & events) != 0 ||
          (events == POLLIN && (descriptor.revents & POLLHUP) != 0)) {
        return TCPIP_L10_OK;
      }
      return TCPIP_L10_SYSTEM;
    }
    if (result == 0) {
      return TCPIP_L10_TIMEOUT;
    }
    if (errno != EINTR) {
      return TCPIP_L10_SYSTEM;
    }
  }
}

static tcpip_l10_status tcpip_l10_prepare_send(int fd) {
#ifdef SO_NOSIGPIPE
  const int enabled = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0) {
    return TCPIP_L10_SYSTEM;
  }
#else
  (void)fd;
#endif
  return TCPIP_L10_OK;
}

static ssize_t tcpip_l10_socket_send(int fd, const uint8_t *data, size_t length) {
#ifdef MSG_NOSIGNAL
  return send(fd, data, length, MSG_NOSIGNAL);
#else
  return send(fd, data, length, 0);
#endif
}

static tcpip_l10_status tcpip_l10_begin_nonblocking(int fd, int *original_flags) {
  int flags;
  do {
    flags = fcntl(fd, F_GETFL);
  } while (flags < 0 && errno == EINTR);
  if (flags < 0) {
    return TCPIP_L10_SYSTEM;
  }
  *original_flags = flags;
  if ((flags & O_NONBLOCK) == 0) {
    int result;
    do {
      result = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    } while (result < 0 && errno == EINTR);
    if (result < 0) {
      return TCPIP_L10_SYSTEM;
    }
  }
  return TCPIP_L10_OK;
}

static tcpip_l10_status tcpip_l10_end_nonblocking(
    int fd, int original_flags, tcpip_l10_status status) {
  if ((original_flags & O_NONBLOCK) != 0) {
    return status;
  }
  int result;
  do {
    result = fcntl(fd, F_SETFL, original_flags);
  } while (result < 0 && errno == EINTR);
  return result < 0 ? TCPIP_L10_SYSTEM : status;
}

static tcpip_l10_status tcpip_l10_send_until(
    int fd,
    const uint8_t *message,
    size_t message_len,
    const struct timespec *deadline,
    size_t *sent) {
  while (*sent < message_len) {
    tcpip_l10_status status = tcpip_l10_wait_fd(fd, POLLOUT, deadline);
    if (status != TCPIP_L10_OK) {
      return status;
    }
    const ssize_t count = tcpip_l10_socket_send(fd, message + *sent, message_len - *sent);
    if (count > 0) {
      *sent += (size_t)count;
    } else if (count == 0) {
      return TCPIP_L10_SYSTEM;
    } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      return TCPIP_L10_SYSTEM;
    }
  }
  return TCPIP_L10_OK;
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
  if (fd < 0 || timeout_ms < 0 || (message == NULL && message_len != 0u)) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  if (message_len == 0u) {
    return TCPIP_L10_OK;
  }
  struct timespec now;
  if (tcpip_l10_now(&now) != 0) {
    return TCPIP_L10_SYSTEM;
  }
  const struct timespec deadline = tcpip_l10_deadline_after(now, timeout_ms);
  int original_flags = 0;
  tcpip_l10_status status = tcpip_l10_begin_nonblocking(fd, &original_flags);
  if (status != TCPIP_L10_OK) {
    return status;
  }
  status = tcpip_l10_prepare_send(fd);
  if (status == TCPIP_L10_OK) {
    status = tcpip_l10_send_until(fd, message, message_len, &deadline, sent);
  }
  return tcpip_l10_end_nonblocking(fd, original_flags, status);
}

static tcpip_l10_status tcpip_l10_recv_until(
    int fd,
    uint8_t *out,
    size_t cap,
    const struct timespec *deadline,
    size_t *received,
    tcpip_l10_message_view *message) {
  for (;;) {
    tcpip_l10_status status = tcpip_l10_parse_message(out, *received, message);
    if (status == TCPIP_L10_OK || status == TCPIP_L10_MALFORMED ||
        status == TCPIP_L10_CAPACITY || status == TCPIP_L10_INVALID_ARGUMENT) {
      return status;
    }
    if (*received == cap) {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_CAPACITY;
    }
    status = tcpip_l10_wait_fd(fd, POLLIN, deadline);
    if (status != TCPIP_L10_OK) {
      memset(message, 0, sizeof(*message));
      return status;
    }
    const ssize_t count = recv(fd, out + *received, cap - *received, 0);
    if (count > 0) {
      *received += (size_t)count;
    } else if (count == 0) {
      memset(message, 0, sizeof(*message));
      return *received == 0u ? TCPIP_L10_EOF : TCPIP_L10_TRUNCATED;
    } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      memset(message, 0, sizeof(*message));
      return TCPIP_L10_SYSTEM;
    }
  }
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
  struct timespec now;
  if (tcpip_l10_now(&now) != 0) {
    return TCPIP_L10_SYSTEM;
  }
  const struct timespec deadline = tcpip_l10_deadline_after(now, timeout_ms);
  int original_flags = 0;
  tcpip_l10_status status = tcpip_l10_begin_nonblocking(fd, &original_flags);
  if (status != TCPIP_L10_OK) {
    return status;
  }
  status = tcpip_l10_recv_until(fd, out, cap, &deadline, received, message);
  return tcpip_l10_end_nonblocking(fd, original_flags, status);
}

tcpip_l10_status tcpip_l10_serve_one(
    int fd,
    uint8_t *request_buffer,
    size_t request_capacity,
    int timeout_ms) {
  if (fd < 0 || request_buffer == NULL || request_capacity == 0u || timeout_ms < 0) {
    return TCPIP_L10_INVALID_ARGUMENT;
  }
  struct timespec now;
  if (tcpip_l10_now(&now) != 0) {
    return TCPIP_L10_SYSTEM;
  }
  const struct timespec deadline = tcpip_l10_deadline_after(now, timeout_ms);
  int original_flags = 0;
  tcpip_l10_status status = tcpip_l10_begin_nonblocking(fd, &original_flags);
  if (status != TCPIP_L10_OK) {
    return status;
  }

  size_t received = 0u;
  tcpip_l10_message_view request;
  memset(&request, 0, sizeof(request));
  status = tcpip_l10_recv_until(
      fd, request_buffer, request_capacity, &deadline, &received, &request);
  if (status == TCPIP_L10_OK && request.kind != TCPIP_L10_MESSAGE_REQUEST) {
    status = TCPIP_L10_MALFORMED;
  }

  static const uint8_t response[] =
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 2\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "\r\n"
      "OK";
  if (status == TCPIP_L10_OK) {
    status = tcpip_l10_prepare_send(fd);
  }
  if (status == TCPIP_L10_OK) {
    size_t sent = 0u;
    status = tcpip_l10_send_until(fd, response, sizeof(response) - 1u, &deadline, &sent);
  }
  return tcpip_l10_end_nonblocking(fd, original_flags, status);
}
