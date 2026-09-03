#define _POSIX_C_SOURCE 200809L

#include "lesson.h"
#include "tcpip/test.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TCPIP_L10_TEST_TIMEOUT_MS 500

typedef struct tcpip_l10_server_context {
  int listener;
  int mode;
  tcpip_l10_status status;
} tcpip_l10_server_context;

static int tcpip_l10_wait_test_fd(int fd, short events, int timeout_ms) {
  struct pollfd descriptor;
  descriptor.fd = fd;
  descriptor.events = events;
  descriptor.revents = 0;
  for (;;) {
    const int result = poll(&descriptor, 1, timeout_ms);
    if (result > 0) {
      return (descriptor.revents & events) != 0;
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

static int tcpip_l10_accept_bounded(int listener) {
  if (!tcpip_l10_wait_test_fd(listener, POLLIN, TCPIP_L10_TEST_TIMEOUT_MS)) {
    return -1;
  }
  int fd;
  do {
    fd = accept(listener, NULL, NULL);
  } while (fd < 0 && errno == EINTR);
  if (fd >= 0) {
    tcpip_l10_disable_sigpipe(fd);
  }
  return fd;
}

static int tcpip_l10_send_fragments(
    int fd, const uint8_t *data, size_t length, size_t fragment_size) {
  size_t offset = 0u;
  while (offset < length) {
    if (!tcpip_l10_wait_test_fd(fd, POLLOUT, TCPIP_L10_TEST_TIMEOUT_MS)) {
      return 0;
    }
    size_t amount = length - offset;
    if (amount > fragment_size) {
      amount = fragment_size;
    }
    const ssize_t count = send(fd, data + offset, amount, 0);
    if (count > 0) {
      offset += (size_t)count;
    } else if (count < 0 && errno == EINTR) {
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

  if (context->mode == 0) {
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
  } else if (context->mode == 1) {
    uint8_t request_buffer[1024];
    context->status = tcpip_l10_serve_one(
        client, request_buffer, sizeof(request_buffer), TCPIP_L10_TEST_TIMEOUT_MS);
  } else {
    static const uint8_t partial[] =
        "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nab";
    context->status = tcpip_l10_send_fragments(
                          client, partial, sizeof(partial) - 1u, 2u)
                          ? TCPIP_L10_OK
                          : TCPIP_L10_SYSTEM;
  }
  tcpip_l10_close_fd(client);
  return NULL;
}

static int tcpip_l10_begin_server(
    int mode,
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
  if (!tcpip_l10_begin_server(0, &context, &thread, &client)) {
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
  if (!tcpip_l10_begin_server(2, &context, &thread, &client)) {
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
  ssize_t count;
  do {
    count = recv(pair[1], copy, sizeof(copy), 0);
  } while (count < 0 && errno == EINTR);
  TCPIP_EXPECT_TRUE(test, count >= 0);
  if (count >= 0) {
    TCPIP_EXPECT_BYTES(test, copy, (size_t)count, bytes, sizeof(bytes) - 1u);
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
  if (!tcpip_l10_begin_server(1, &context, &thread, &client)) {
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
  return tcpip_test_finish(&test);
}
