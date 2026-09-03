#define _POSIX_C_SOURCE 200809L

#include "lesson.h"
#include "tcpip/test.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TEST_TIMEOUT_MS 1000

typedef struct server_context {
  int listen_fd;
  int timeout_ms;
  tcpip_l08_status status;
} server_context;

static void close_if_open(int *fd) {
  if (*fd >= 0) {
    (void)close(*fd);
    *fd = -1;
  }
}

static int make_loopback_listener(
    tcpip_test_context *test, struct sockaddr_in *address) {
  struct in_addr loopback;
  int listen_fd = -1;
  int reuse = 1;
  socklen_t address_length = (socklen_t)sizeof(*address);

  if (inet_pton(AF_INET, "127.0.0.1", &loopback) != 1) {
    TCPIP_FAIL(test, "inet_pton failed for 127.0.0.1");
    return -1;
  }
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    TCPIP_FAIL(test, "socket failed for loopback listener");
    return -1;
  }
  (void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  memset(address, 0, sizeof(*address));
  address->sin_family = AF_INET;
  address->sin_addr = loopback;
  address->sin_port = htons(0U);
  if (bind(listen_fd, (const struct sockaddr *)address, sizeof(*address)) != 0) {
    TCPIP_FAIL(test, "bind failed for 127.0.0.1 port 0");
    close_if_open(&listen_fd);
    return -1;
  }
  if (getsockname(listen_fd, (struct sockaddr *)address, &address_length) != 0) {
    TCPIP_FAIL(test, "getsockname failed for loopback listener");
    close_if_open(&listen_fd);
    return -1;
  }
  if (address_length != (socklen_t)sizeof(*address)
      || address->sin_addr.s_addr != loopback.s_addr
      || ntohs(address->sin_port) == 0U) {
    TCPIP_FAIL(test, "listener did not receive a loopback ephemeral port");
    close_if_open(&listen_fd);
    return -1;
  }
  if (listen(listen_fd, 1) != 0) {
    TCPIP_FAIL(test, "listen failed for loopback listener");
    close_if_open(&listen_fd);
    return -1;
  }
  return listen_fd;
}

static int connect_loopback(
    tcpip_test_context *test, const struct sockaddr_in *address) {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (client_fd < 0) {
    TCPIP_FAIL(test, "socket failed for loopback client");
    return -1;
  }
  if (connect(client_fd, (const struct sockaddr *)address, sizeof(*address)) != 0) {
    TCPIP_FAIL(test, "connect failed for loopback client");
    close_if_open(&client_fd);
    return -1;
  }
  return client_fd;
}

static void *serve_thread(void *argument) {
  server_context *context = (server_context *)argument;

  context->status = tcpip_l08_serve_one(context->listen_fd, context->timeout_ms);
  return NULL;
}

static void expect_status(
    tcpip_test_context *test, tcpip_l08_status actual, tcpip_l08_status expected) {
  TCPIP_EXPECT_U32(test, (uint32_t)actual, (uint32_t)expected);
}

static void test_argument_contract(tcpip_test_context *test) {
  size_t out_len = 99U;
  uint8_t byte = 0U;

  expect_status(test, tcpip_l08_send_all(-1, &byte, 1U, TEST_TIMEOUT_MS),
                TCPIP_L08_INVALID_ARGUMENT);
  expect_status(test, tcpip_l08_send_all(0, NULL, 1U, TEST_TIMEOUT_MS),
                TCPIP_L08_INVALID_ARGUMENT);
  expect_status(test, tcpip_l08_recv_exact(0, NULL, 1U, TEST_TIMEOUT_MS),
                TCPIP_L08_INVALID_ARGUMENT);
  expect_status(test, tcpip_l08_send_frame(0, &byte, 1U, -1),
                TCPIP_L08_INVALID_ARGUMENT);
  expect_status(test, tcpip_l08_recv_frame(0, NULL, 1U, &out_len, TEST_TIMEOUT_MS),
                TCPIP_L08_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, out_len, 0U);
  expect_status(test, tcpip_l08_recv_frame(0, &byte, 1U, NULL, TEST_TIMEOUT_MS),
                TCPIP_L08_INVALID_ARGUMENT);
  expect_status(test, tcpip_l08_serve_one(-1, TEST_TIMEOUT_MS),
                TCPIP_L08_INVALID_ARGUMENT);
}

static void test_multiple_frames(tcpip_test_context *test) {
  static const uint8_t first[] = {0x00U, 0x41U, 0xffU};
  static const uint8_t second[] = {0x10U, 0x20U, 0x30U, 0x40U, 0x50U};
  uint8_t received[16];
  size_t received_len = 99U;
  int sockets[2] = {-1, -1};

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    TCPIP_FAIL(test, "socketpair failed for multiple-frame test");
    return;
  }
  expect_status(test, tcpip_l08_send_frame(
                         sockets[0], first, sizeof(first), TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  expect_status(test, tcpip_l08_send_frame(
                         sockets[0], second, sizeof(second), TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  expect_status(test, tcpip_l08_recv_frame(
                         sockets[1], received, sizeof(received), &received_len,
                         TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  TCPIP_EXPECT_BYTES(test, received, received_len, first, sizeof(first));
  expect_status(test, tcpip_l08_recv_frame(
                         sockets[1], received, sizeof(received), &received_len,
                         TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  TCPIP_EXPECT_BYTES(test, received, received_len, second, sizeof(second));
  close_if_open(&sockets[0]);
  close_if_open(&sockets[1]);
}

static void test_oversize_and_eof(tcpip_test_context *test) {
  static const uint8_t oversized[] = {
      0x00U, 0x00U, 0x00U, 0x05U, 'a', 'b', 'c', 'd', 'e'};
  uint8_t received[8] = {0U};
  size_t received_len = 77U;
  int sockets[2] = {-1, -1};

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    TCPIP_FAIL(test, "socketpair failed for capacity test");
    return;
  }
  expect_status(test, tcpip_l08_send_all(
                         sockets[0], oversized, sizeof(oversized), TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  expect_status(test, tcpip_l08_recv_frame(
                         sockets[1], received, 4U, &received_len, TEST_TIMEOUT_MS),
                TCPIP_L08_CAPACITY);
  TCPIP_EXPECT_SIZE(test, received_len, 0U);
  expect_status(test, tcpip_l08_recv_exact(
                         sockets[1], received, 5U, TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  TCPIP_EXPECT_BYTES(test, received, 5U, oversized + 4U, 5U);
  close_if_open(&sockets[0]);
  close_if_open(&sockets[1]);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    TCPIP_FAIL(test, "socketpair failed for EOF test");
    return;
  }
  close_if_open(&sockets[0]);
  expect_status(test, tcpip_l08_recv_exact(
                         sockets[1], received, 1U, TEST_TIMEOUT_MS),
                TCPIP_L08_EOF);
  close_if_open(&sockets[1]);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    TCPIP_FAIL(test, "socketpair failed for truncated-input test");
    return;
  }
  expect_status(test, tcpip_l08_send_all(
                         sockets[0], oversized, 2U, TEST_TIMEOUT_MS),
                TCPIP_L08_OK);
  close_if_open(&sockets[0]);
  expect_status(test, tcpip_l08_recv_exact(
                         sockets[1], received, 4U, TEST_TIMEOUT_MS),
                TCPIP_L08_TRUNCATED);
  close_if_open(&sockets[1]);
}

static void test_fragmented_loopback_echo(tcpip_test_context *test) {
  static const uint8_t fragments[][3] = {
      {0x00U, 0x00U, 0x00U},
      {0x06U, 's', 't'},
      {'r', 'e', 'a'},
      {'m', 0x00U, 0x00U}};
  static const size_t fragment_lengths[] = {3U, 3U, 3U, 1U};
  static const uint8_t expected[] = {'s', 't', 'r', 'e', 'a', 'm'};
  struct sockaddr_in address;
  server_context context;
  pthread_t thread;
  uint8_t echoed[16];
  size_t echoed_len = 0U;
  size_t index;
  int client_fd = -1;
  int thread_started = 0;

  context.listen_fd = make_loopback_listener(test, &address);
  context.timeout_ms = TEST_TIMEOUT_MS;
  context.status = TCPIP_L08_TODO;
  if (context.listen_fd < 0) {
    return;
  }
  if (pthread_create(&thread, NULL, serve_thread, &context) != 0) {
    TCPIP_FAIL(test, "pthread_create failed for echo server");
    close_if_open(&context.listen_fd);
    return;
  }
  thread_started = 1;
  client_fd = connect_loopback(test, &address);
  if (client_fd >= 0) {
    for (index = 0U; index < sizeof(fragment_lengths) / sizeof(fragment_lengths[0]);
         index += 1U) {
      tcpip_l08_status status = tcpip_l08_send_all(
          client_fd, fragments[index], fragment_lengths[index], TEST_TIMEOUT_MS);
      expect_status(test, status, TCPIP_L08_OK);
      if (status != TCPIP_L08_OK) {
        break;
      }
    }
    expect_status(test, tcpip_l08_recv_frame(
                           client_fd, echoed, sizeof(echoed), &echoed_len,
                           TEST_TIMEOUT_MS),
                  TCPIP_L08_OK);
    TCPIP_EXPECT_BYTES(test, echoed, echoed_len, expected, sizeof(expected));
  }
  close_if_open(&client_fd);
  if (thread_started != 0 && pthread_join(thread, NULL) != 0) {
    TCPIP_FAIL(test, "pthread_join failed for echo server");
  }
  expect_status(test, context.status, TCPIP_L08_OK);
  close_if_open(&context.listen_fd);
}

static void test_server_status_propagation(tcpip_test_context *test) {
  static const uint8_t partial_header[] = {0x00U, 0x00U};
  struct sockaddr_in address;
  server_context context;
  pthread_t thread;
  int client_fd = -1;

  context.listen_fd = make_loopback_listener(test, &address);
  context.timeout_ms = TEST_TIMEOUT_MS;
  context.status = TCPIP_L08_TODO;
  if (context.listen_fd < 0) {
    return;
  }
  if (pthread_create(&thread, NULL, serve_thread, &context) != 0) {
    TCPIP_FAIL(test, "pthread_create failed for status server");
    close_if_open(&context.listen_fd);
    return;
  }
  client_fd = connect_loopback(test, &address);
  if (client_fd >= 0) {
    expect_status(test, tcpip_l08_send_all(
                           client_fd, partial_header, sizeof(partial_header),
                           TEST_TIMEOUT_MS),
                  TCPIP_L08_OK);
  }
  close_if_open(&client_fd);
  if (pthread_join(thread, NULL) != 0) {
    TCPIP_FAIL(test, "pthread_join failed for status server");
  }
  expect_status(test, context.status, TCPIP_L08_TRUNCATED);
  close_if_open(&context.listen_fd);
}

static void test_bounded_timeouts(tcpip_test_context *test) {
  struct sockaddr_in address;
  uint8_t byte = 0U;
  int sockets[2] = {-1, -1};
  int listen_fd = make_loopback_listener(test, &address);

  if (listen_fd >= 0) {
    expect_status(test, tcpip_l08_serve_one(listen_fd, 20), TCPIP_L08_TIMEOUT);
    close_if_open(&listen_fd);
  }
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    TCPIP_FAIL(test, "socketpair failed for timeout test");
    return;
  }
  expect_status(test, tcpip_l08_recv_exact(sockets[0], &byte, 1U, 20),
                TCPIP_L08_TIMEOUT);
  close_if_open(&sockets[0]);
  close_if_open(&sockets[1]);
}

int main(void) {
  tcpip_test_context test;

  tcpip_test_begin(&test, "lesson 08 stream sockets");
  test_argument_contract(&test);
  test_multiple_frames(&test);
  test_oversize_and_eof(&test);
  test_fragmented_loopback_echo(&test);
  test_server_status_propagation(&test);
  test_bounded_timeouts(&test);
  return tcpip_test_finish(&test);
}
