#define _POSIX_C_SOURCE 200809L

#include "lesson.h"
#include "tcpip/test.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TCPIP_L10_TEST_TIMEOUT_MS 500

typedef enum tcpip_l10_server_mode {
  TCPIP_L10_SERVER_FRAGMENTED_RESPONSE = 0,
  TCPIP_L10_SERVER_SERVE = 1,
  TCPIP_L10_SERVER_PARTIAL_RESPONSE = 2,
  TCPIP_L10_SERVER_SERVE_SHORT_TIMEOUT = 3,
  TCPIP_L10_SERVER_SERVE_SMALL_BUFFER = 4
} tcpip_l10_server_mode;

typedef struct tcpip_l10_server_context {
  int listener;
  tcpip_l10_server_mode mode;
  tcpip_l10_status status;
} tcpip_l10_server_context;

static int tcpip_l10_test_deadline_after(
    int timeout_ms, struct timespec *deadline) {
  if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) {
    return 0;
  }
  deadline->tv_sec += (time_t)(timeout_ms / 1000);
  deadline->tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
  if (deadline->tv_nsec >= 1000000000L) {
    deadline->tv_sec += 1;
    deadline->tv_nsec -= 1000000000L;
  }
  return 1;
}

static int tcpip_l10_test_remaining_ms(const struct timespec *deadline) {
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
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

static int tcpip_l10_wait_test_fd_until(
    int fd, short events, const struct timespec *deadline) {
  struct pollfd descriptor;
  descriptor.fd = fd;
  descriptor.events = events;
  descriptor.revents = 0;
  for (;;) {
    const int remaining_ms = tcpip_l10_test_remaining_ms(deadline);
    if (remaining_ms <= 0) {
      return 0;
    }
    descriptor.revents = 0;
    const int result = poll(&descriptor, 1, remaining_ms);
    if (result > 0) {
      if ((descriptor.revents & POLLNVAL) != 0) {
        return 0;
      }
      if ((descriptor.revents & (events | POLLERR | POLLHUP)) != 0) {
        return 1;
      }
      continue;
    }
    if (result == 0 || errno != EINTR) {
      return 0;
    }
  }
}

static void tcpip_l10_close_fd(int fd) {
  if (fd >= 0) {
    (void)close(fd);
  }
}

static void tcpip_l10_disable_sigpipe(int fd) {
#ifdef SO_NOSIGPIPE
  int enabled = 1;
  (void)setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#else
  (void)fd;
#endif
}

static int tcpip_l10_open_listener(uint16_t *port) {
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    return -1;
  }
  int enabled = 1;
  (void)setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(UINT32_C(0x7f000001));
  address.sin_port = htons(0u);
  if (bind(listener, (const struct sockaddr *)&address, sizeof(address)) != 0 ||
      listen(listener, 1) != 0) {
    tcpip_l10_close_fd(listener);
    return -1;
  }
  socklen_t address_len = (socklen_t)sizeof(address);
  if (getsockname(listener, (struct sockaddr *)&address, &address_len) != 0) {
    tcpip_l10_close_fd(listener);
    return -1;
  }
  *port = ntohs(address.sin_port);
  return listener;
}

static int tcpip_l10_connect_loopback(uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return -1;
  }
  tcpip_l10_disable_sigpipe(fd);
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(UINT32_C(0x7f000001));
  address.sin_port = htons(port);
  if (connect(fd, (const struct sockaddr *)&address, sizeof(address)) != 0) {
    tcpip_l10_close_fd(fd);
    return -1;
  }
  return fd;
}

static int tcpip_l10_get_flags(int fd, int *flags) {
  int result;
  do {
    result = fcntl(fd, F_GETFL);
  } while (result < 0 && errno == EINTR);
  if (result < 0) {
    return 0;
  }
  *flags = result;
  return 1;
}

static int tcpip_l10_set_flags(int fd, int flags) {
  int result;
  do {
    result = fcntl(fd, F_SETFL, flags);
  } while (result < 0 && errno == EINTR);
  return result == 0;
}

static int tcpip_l10_accept_bounded(int listener) {
  struct timespec deadline;
  int original_flags = 0;
  int changed_flags = 0;
  int fd = -1;

  if (!tcpip_l10_test_deadline_after(TCPIP_L10_TEST_TIMEOUT_MS, &deadline) ||
      !tcpip_l10_get_flags(listener, &original_flags)) {
    return -1;
  }
  if ((original_flags & O_NONBLOCK) == 0) {
    if (!tcpip_l10_set_flags(listener, original_flags | O_NONBLOCK)) {
      return -1;
    }
    changed_flags = 1;
  }
  for (;;) {
    if (!tcpip_l10_wait_test_fd_until(listener, POLLIN, &deadline)) {
      break;
    }
    fd = accept(listener, NULL, NULL);
    if (fd >= 0) {
      break;
    }
    if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      break;
    }
  }
  if (changed_flags != 0 && !tcpip_l10_set_flags(listener, original_flags)) {
    tcpip_l10_close_fd(fd);
    return -1;
  }
  if (fd >= 0) {
    tcpip_l10_disable_sigpipe(fd);
  }
  return fd;
}

static int tcpip_l10_send_fragments(
    int fd, const uint8_t *data, size_t length, size_t fragment_size) {
  struct timespec deadline;
  size_t offset = 0u;
  if (!tcpip_l10_test_deadline_after(TCPIP_L10_TEST_TIMEOUT_MS, &deadline)) {
    return 0;
  }
  while (offset < length) {
    if (!tcpip_l10_wait_test_fd_until(fd, POLLOUT, &deadline)) {
      return 0;
    }
    size_t amount = length - offset;
    if (amount > fragment_size) {
      amount = fragment_size;
    }
#ifdef MSG_NOSIGNAL
    const ssize_t count = send(fd, data + offset, amount, MSG_NOSIGNAL);
#else
    const ssize_t count = send(fd, data + offset, amount, 0);
#endif
    if (count > 0) {
      offset += (size_t)count;
    } else if (count < 0 &&
               (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    } else {
      return 0;
    }
  }
  return 1;
}

static int tcpip_l10_recv_exact_test(int fd, uint8_t *out, size_t length) {
  struct timespec deadline;
  size_t offset = 0u;
  if (!tcpip_l10_test_deadline_after(TCPIP_L10_TEST_TIMEOUT_MS, &deadline)) {
    return 0;
  }
  while (offset < length) {
    if (!tcpip_l10_wait_test_fd_until(fd, POLLIN, &deadline)) {
      return 0;
    }
#ifdef MSG_DONTWAIT
    const ssize_t count = recv(fd, out + offset, length - offset, MSG_DONTWAIT);
#else
    const ssize_t count = recv(fd, out + offset, length - offset, 0);
#endif
    if (count > 0) {
      offset += (size_t)count;
    } else if (count < 0 &&
               (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    } else {
      return 0;
    }
  }
  return 1;
}

static void *tcpip_l10_server_thread(void *opaque) {
  tcpip_l10_server_context *context = (tcpip_l10_server_context *)opaque;
  int client = tcpip_l10_accept_bounded(context->listener);
  if (client < 0) {
    context->status = TCPIP_L10_SYSTEM;
    return NULL;
  }

  if (context->mode == TCPIP_L10_SERVER_FRAGMENTED_RESPONSE) {
    static const uint8_t response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "Connection: close\r\n"
        "\r\n"
        "hello";
    context->status = tcpip_l10_send_fragments(
                          client, response, sizeof(response) - 1u, 3u)
                          ? TCPIP_L10_OK
                          : TCPIP_L10_SYSTEM;
  } else if (context->mode == TCPIP_L10_SERVER_PARTIAL_RESPONSE) {
    static const uint8_t partial[] =
        "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nab";
    context->status = tcpip_l10_send_fragments(
                          client, partial, sizeof(partial) - 1u, 2u)
                          ? TCPIP_L10_OK
                          : TCPIP_L10_SYSTEM;
  } else {
    uint8_t request_buffer[1024];
    const int timeout_ms =
        context->mode == TCPIP_L10_SERVER_SERVE_SHORT_TIMEOUT
            ? 20
            : TCPIP_L10_TEST_TIMEOUT_MS;
    const size_t capacity =
        context->mode == TCPIP_L10_SERVER_SERVE_SMALL_BUFFER
            ? 32u
            : sizeof(request_buffer);
    context->status = tcpip_l10_serve_one(
        client, request_buffer, capacity, timeout_ms);
  }
  tcpip_l10_close_fd(client);
  return NULL;
}

static int tcpip_l10_begin_server(
    tcpip_l10_server_mode mode,
    tcpip_l10_server_context *context,
    pthread_t *thread,
    int *client) {
  uint16_t port = 0u;
  context->listener = tcpip_l10_open_listener(&port);
  context->mode = mode;
  context->status = TCPIP_L10_SYSTEM;
  if (context->listener < 0) {
    return 0;
  }
  if (pthread_create(thread, NULL, tcpip_l10_server_thread, context) != 0) {
    tcpip_l10_close_fd(context->listener);
    context->listener = -1;
    return 0;
  }
  *client = tcpip_l10_connect_loopback(port);
  if (*client < 0) {
    tcpip_l10_close_fd(context->listener);
    (void)pthread_join(*thread, NULL);
    context->listener = -1;
    return 0;
  }
  return 1;
}

static void tcpip_l10_finish_server(
    tcpip_l10_server_context *context, pthread_t thread, int client) {
  tcpip_l10_close_fd(client);
  (void)pthread_join(thread, NULL);
  tcpip_l10_close_fd(context->listener);
  context->listener = -1;
}

static int tcpip_l10_span_is(tcpip_l10_span span, const char *text) {
  const size_t length = strlen(text);
  return span.length == length && memcmp(span.data, text, length) == 0;
}

static void tcpip_l10_test_build_and_parse(tcpip_test_context *test) {
  uint8_t output[256];
  size_t written = 99u;
  static const uint8_t path[] = "/submit?q=1";
  static const uint8_t body[] = "abc";
  static const uint8_t expected[] =
      "POST /submit?q=1 HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 3\r\n"
      "Connection: close\r\n"
      "\r\n"
      "abc";
  tcpip_l10_status status = tcpip_l10_build_request(
      path, sizeof(path) - 1u, body, sizeof(body) - 1u, output, sizeof(output), &written);
  TCPIP_EXPECT_U32(test, status, TCPIP_L10_OK);
  TCPIP_EXPECT_BYTES(test, output, written, expected, sizeof(expected) - 1u);

  tcpip_l10_message_view message;
  memset(&message, 0xa5, sizeof(message));
  status = tcpip_l10_parse_message(output, written, &message);
  TCPIP_EXPECT_U32(test, status, TCPIP_L10_OK);
  TCPIP_EXPECT_U32(test, message.kind, TCPIP_L10_MESSAGE_REQUEST);
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.method, "POST"));
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.target, "/submit?q=1"));
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.version, "HTTP/1.1"));
  TCPIP_EXPECT_SIZE(test, message.header_count, 3u);
  TCPIP_EXPECT_SIZE(test, message.content_length, 3u);
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.body, "abc"));
  TCPIP_EXPECT_SIZE(test, message.message_length, written);
}

static void tcpip_l10_test_validation_and_initialization(tcpip_test_context *test) {
  size_t written = 77u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_build_request(NULL, 1u, NULL, 0u, NULL, 0u, &written),
      TCPIP_L10_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, written, 0u);

  uint8_t output[8];
  static const uint8_t path[] = "/";
  written = 77u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_build_request(path, 1u, NULL, 0u, output, sizeof(output), &written),
      TCPIP_L10_CAPACITY);
  TCPIP_EXPECT_SIZE(test, written, 0u);

  tcpip_l10_message_view message;
  memset(&message, 0xa5, sizeof(message));
  TCPIP_EXPECT_U32(
      test, tcpip_l10_parse_message(NULL, 1u, &message), TCPIP_L10_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, message.header_count, 0u);
  TCPIP_EXPECT_SIZE(test, message.body.length, 0u);
  TCPIP_EXPECT_SIZE(test, message.message_length, 0u);

  size_t sent = 55u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_send_message(-1, NULL, 0u, TCPIP_L10_TEST_TIMEOUT_MS, &sent),
      TCPIP_L10_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, sent, 0u);

  size_t received = 44u;
  memset(&message, 0xa5, sizeof(message));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_recv_message(
          -1, output, sizeof(output), TCPIP_L10_TEST_TIMEOUT_MS, &received, &message),
      TCPIP_L10_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, received, 0u);
  TCPIP_EXPECT_SIZE(test, message.header_count, 0u);
}

static void tcpip_l10_expect_parse_status(
    tcpip_test_context *test,
    const uint8_t *input,
    size_t length,
    tcpip_l10_status expected) {
  tcpip_l10_message_view message;
  memset(&message, 0xa5, sizeof(message));
  const tcpip_l10_status actual = tcpip_l10_parse_message(input, length, &message);
  TCPIP_EXPECT_U32(test, actual, expected);
  if (actual != TCPIP_L10_OK) {
    TCPIP_EXPECT_SIZE(test, message.header_count, 0u);
    TCPIP_EXPECT_SIZE(test, message.body.length, 0u);
    TCPIP_EXPECT_SIZE(test, message.message_length, 0u);
  }
}

static void tcpip_l10_test_rejected_messages(tcpip_test_context *test) {
  static const uint8_t truncated[] =
      "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nab";
  static const uint8_t chunked[] =
      "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
  static const uint8_t folded[] =
      "GET / HTTP/1.1\r\nX-Test: one\r\n two\r\n\r\n";
  static const uint8_t conflicting[] =
      "HTTP/1.1 200 OK\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nxx";
  static const uint8_t invalid_length[] =
      "HTTP/1.1 200 OK\r\nContent-Length: +2\r\n\r\nxx";
  static const uint8_t bare_lf[] = "GET / HTTP/1.1\nHost: localhost\n\n";
  static const uint8_t huge_body[] =
      "HTTP/1.1 200 OK\r\nContent-Length: 8193\r\n\r\n";
  tcpip_l10_expect_parse_status(
      test, truncated, sizeof(truncated) - 1u, TCPIP_L10_TRUNCATED);
  tcpip_l10_expect_parse_status(
      test, chunked, sizeof(chunked) - 1u, TCPIP_L10_MALFORMED);
  tcpip_l10_expect_parse_status(test, folded, sizeof(folded) - 1u, TCPIP_L10_MALFORMED);
  tcpip_l10_expect_parse_status(
      test, conflicting, sizeof(conflicting) - 1u, TCPIP_L10_MALFORMED);
  tcpip_l10_expect_parse_status(
      test, invalid_length, sizeof(invalid_length) - 1u, TCPIP_L10_MALFORMED);
  tcpip_l10_expect_parse_status(test, bare_lf, sizeof(bare_lf) - 1u, TCPIP_L10_MALFORMED);
  tcpip_l10_expect_parse_status(
      test, huge_body, sizeof(huge_body) - 1u, TCPIP_L10_CAPACITY);

  uint8_t oversized[TCPIP_L10_MAX_HEADER_BYTES];
  memset(oversized, (int)'A', sizeof(oversized));
  tcpip_l10_expect_parse_status(test, oversized, sizeof(oversized), TCPIP_L10_CAPACITY);

  static const uint8_t too_many[] =
      "GET / HTTP/1.1\r\n"
      "A: 0\r\nB: 0\r\nC: 0\r\nD: 0\r\nE: 0\r\nF: 0\r\nG: 0\r\nH: 0\r\n"
      "I: 0\r\nJ: 0\r\nK: 0\r\nL: 0\r\nM: 0\r\nN: 0\r\nO: 0\r\nP: 0\r\n"
      "Q: 0\r\n\r\n";
  tcpip_l10_expect_parse_status(
      test, too_many, sizeof(too_many) - 1u, TCPIP_L10_CAPACITY);
}

static void tcpip_l10_test_fragmented_receive(tcpip_test_context *test) {
  tcpip_l10_server_context context;
  pthread_t thread;
  int client = -1;
  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_FRAGMENTED_RESPONSE, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start fragmented loopback server");
    return;
  }

  uint8_t buffer[256];
  size_t received = 99u;
  tcpip_l10_message_view message;
  const tcpip_l10_status status = tcpip_l10_recv_message(
      client, buffer, sizeof(buffer), TCPIP_L10_TEST_TIMEOUT_MS, &received, &message);
  tcpip_l10_finish_server(&context, thread, client);
  TCPIP_EXPECT_U32(test, status, TCPIP_L10_OK);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_OK);
  TCPIP_EXPECT_U32(test, message.kind, TCPIP_L10_MESSAGE_RESPONSE);
  TCPIP_EXPECT_U32(test, message.status_code, 200u);
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.reason, "OK"));
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.body, "hello"));
  TCPIP_EXPECT_SIZE(test, message.message_length, received);
}

static void tcpip_l10_test_premature_eof(tcpip_test_context *test) {
  tcpip_l10_server_context context;
  pthread_t thread;
  int client = -1;
  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_PARTIAL_RESPONSE, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start premature-EOF loopback server");
    return;
  }
  uint8_t buffer[256];
  size_t received = 0u;
  tcpip_l10_message_view message;
  const tcpip_l10_status status = tcpip_l10_recv_message(
      client, buffer, sizeof(buffer), TCPIP_L10_TEST_TIMEOUT_MS, &received, &message);
  tcpip_l10_finish_server(&context, thread, client);
  TCPIP_EXPECT_U32(test, status, TCPIP_L10_TRUNCATED);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_OK);
  TCPIP_EXPECT_TRUE(test, received > 0u);
  TCPIP_EXPECT_SIZE(test, message.message_length, 0u);
}

static void tcpip_l10_test_socket_outcomes(tcpip_test_context *test) {
  int pair[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
    TCPIP_FAIL(test, "could not create socket pair");
    return;
  }
  tcpip_l10_disable_sigpipe(pair[0]);
  tcpip_l10_disable_sigpipe(pair[1]);

  static const uint8_t bytes[] = "message";
  size_t sent = 99u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_send_message(
          pair[0], bytes, sizeof(bytes) - 1u, TCPIP_L10_TEST_TIMEOUT_MS, &sent),
      TCPIP_L10_OK);
  TCPIP_EXPECT_SIZE(test, sent, sizeof(bytes) - 1u);
  uint8_t copy[sizeof(bytes) - 1u];
  const int received_all = tcpip_l10_recv_exact_test(pair[1], copy, sizeof(copy));
  TCPIP_EXPECT_TRUE(test, received_all);
  if (received_all != 0) {
    TCPIP_EXPECT_BYTES(test, copy, sizeof(copy), bytes, sizeof(bytes) - 1u);
  }

  uint8_t buffer[64];
  size_t received = 99u;
  tcpip_l10_message_view message;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_recv_message(pair[0], buffer, sizeof(buffer), 20, &received, &message),
      TCPIP_L10_TIMEOUT);
  TCPIP_EXPECT_SIZE(test, received, 0u);
  TCPIP_EXPECT_SIZE(test, message.message_length, 0u);

  tcpip_l10_close_fd(pair[1]);
  received = 99u;
  memset(&message, 0xa5, sizeof(message));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l10_recv_message(
          pair[0], buffer, sizeof(buffer), TCPIP_L10_TEST_TIMEOUT_MS, &received, &message),
      TCPIP_L10_EOF);
  TCPIP_EXPECT_SIZE(test, received, 0u);
  TCPIP_EXPECT_SIZE(test, message.message_length, 0u);
  tcpip_l10_close_fd(pair[0]);
}

static void tcpip_l10_test_serve_one(tcpip_test_context *test) {
  tcpip_l10_server_context context;
  pthread_t thread;
  int client = -1;
  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_SERVE, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start serve-one loopback server");
    return;
  }
  static const uint8_t request[] =
      "POST /demo HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 4\r\n"
      "\r\n"
      "ping";
  if (!tcpip_l10_send_fragments(client, request, sizeof(request) - 1u, 2u)) {
    TCPIP_FAIL(test, "could not send fragmented request");
    tcpip_l10_finish_server(&context, thread, client);
    return;
  }

  uint8_t response[256];
  size_t received = 0u;
  tcpip_l10_message_view message;
  const tcpip_l10_status status = tcpip_l10_recv_message(
      client, response, sizeof(response), TCPIP_L10_TEST_TIMEOUT_MS, &received, &message);
  tcpip_l10_finish_server(&context, thread, client);
  TCPIP_EXPECT_U32(test, status, TCPIP_L10_OK);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_OK);
  TCPIP_EXPECT_U32(test, message.status_code, 200u);
  TCPIP_EXPECT_TRUE(test, tcpip_l10_span_is(message.body, "OK"));
  TCPIP_EXPECT_SIZE(test, message.content_length, 2u);
}

static void tcpip_l10_finish_joined_server(
    tcpip_test_context *test,
    tcpip_l10_server_context *context,
    pthread_t thread,
    int client) {
  if (pthread_join(thread, NULL) != 0) {
    TCPIP_FAIL(test, "could not join serve-one server");
  }
  tcpip_l10_close_fd(client);
  tcpip_l10_close_fd(context->listener);
  context->listener = -1;
}

static void tcpip_l10_test_serve_one_failure_propagation(tcpip_test_context *test) {
  static const uint8_t truncated[] =
      "POST /demo HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 4\r\n"
      "\r\n"
      "pi";
  static const uint8_t malformed[] =
      "POST /demo HTTP/1.1\nHost: localhost\n\n";
  static const uint8_t too_large_for_buffer[] =
      "POST /demo HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 1\r\n"
      "Connection: close\r\n"
      "\r\n"
      "x";

  tcpip_l10_server_context context;
  pthread_t thread;
  int client = -1;
  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_SERVE, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start truncated serve-one server");
    return;
  }
  TCPIP_EXPECT_TRUE(test, tcpip_l10_send_fragments(
                              client, truncated, sizeof(truncated) - 1u, 2u));
  tcpip_l10_close_fd(client);
  client = -1;
  tcpip_l10_finish_server(&context, thread, client);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_TRUNCATED);

  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_SERVE_SHORT_TIMEOUT, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start timeout serve-one server");
    return;
  }
  tcpip_l10_finish_joined_server(test, &context, thread, client);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_TIMEOUT);

  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_SERVE, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start malformed serve-one server");
    return;
  }
  (void)tcpip_l10_send_fragments(
      client, malformed, sizeof(malformed) - 1u, 2u);
  tcpip_l10_finish_joined_server(test, &context, thread, client);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_MALFORMED);

  if (!tcpip_l10_begin_server(
          TCPIP_L10_SERVER_SERVE_SMALL_BUFFER, &context, &thread, &client)) {
    TCPIP_FAIL(test, "could not start capacity serve-one server");
    return;
  }
  (void)tcpip_l10_send_fragments(
      client, too_large_for_buffer, sizeof(too_large_for_buffer) - 1u, 3u);
  tcpip_l10_finish_joined_server(test, &context, thread, client);
  TCPIP_EXPECT_U32(test, context.status, TCPIP_L10_CAPACITY);
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "lesson 10 HTTP");

  uint8_t probe[128];
  size_t probe_length = 0u;
  static const uint8_t probe_path[] = "/";
  if (tcpip_l10_build_request(
          probe_path, sizeof(probe_path) - 1u, NULL, 0u,
          probe, sizeof(probe), &probe_length) == TCPIP_L10_TODO) {
    TCPIP_FAIL(&test, "starter still returns TCPIP_L10_TODO");
    return tcpip_test_finish(&test);
  }

  tcpip_l10_test_build_and_parse(&test);
  tcpip_l10_test_validation_and_initialization(&test);
  tcpip_l10_test_rejected_messages(&test);
  tcpip_l10_test_fragmented_receive(&test);
  tcpip_l10_test_premature_eof(&test);
  tcpip_l10_test_socket_outcomes(&test);
  tcpip_l10_test_serve_one(&test);
  tcpip_l10_test_serve_one_failure_propagation(&test);
  return tcpip_test_finish(&test);
}
