#include "lab.h"
#include "tcpip/test.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TEST_PAYLOAD_SIZE (256U * 1024U)

typedef struct peer_context {
  int fd;
  size_t length;
  uint8_t sent[TEST_PAYLOAD_SIZE];
  uint8_t received[TEST_PAYLOAD_SIZE];
  size_t sent_count;
  size_t received_count;
  int error;
} peer_context;

static int set_nonblocking_raw(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void *peer_exchange(void *argument) {
  peer_context *peer = (peer_context *)argument;

  if (set_nonblocking_raw(peer->fd) < 0) {
    peer->error = errno;
    return NULL;
  }
  while (peer->sent_count < peer->length || peer->received_count < peer->length) {
    struct pollfd descriptor;
    int result;

    descriptor.fd = peer->fd;
    descriptor.events = POLLIN;
    if (peer->sent_count < peer->length) {
      descriptor.events |= POLLOUT;
    }
    descriptor.revents = 0;
    do {
      result = poll(&descriptor, 1, 3000);
    } while (result < 0 && errno == EINTR);
    if (result <= 0) {
      peer->error = result == 0 ? ETIMEDOUT : errno;
      return NULL;
    }
    if ((descriptor.revents & POLLOUT) != 0 && peer->sent_count < peer->length) {
      size_t remaining = peer->length - peer->sent_count;
      size_t quantum = remaining > 701U ? 701U : remaining;
      ssize_t count = send(
          peer->fd, peer->sent + peer->sent_count, quantum, MSG_NOSIGNAL);
      if (count > 0) {
        peer->sent_count += (size_t)count;
      } else if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        peer->error = errno;
        return NULL;
      }
    }
    if ((descriptor.revents & POLLIN) != 0 && peer->received_count < peer->length) {
      size_t remaining = peer->length - peer->received_count;
      size_t quantum = remaining > 389U ? 389U : remaining;
      ssize_t count = recv(
          peer->fd, peer->received + peer->received_count, quantum, 0);
      if (count > 0) {
        peer->received_count += (size_t)count;
      } else if (count == 0) {
        peer->error = ECONNRESET;
        return NULL;
      } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        peer->error = errno;
        return NULL;
      }
    }
    if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0) {
      peer->error = EIO;
      return NULL;
    }
  }
  return NULL;
}

static void fill_payload(peer_context *peer, size_t length) {
  memset(peer, 0, sizeof(*peer));
  peer->length = length;
  for (size_t index = 0U; index < length; index += 1U) {
    peer->sent[index] = (uint8_t)((index * 37U + 11U) & 0xffU);
  }
}

static int count_open_fds(void) {
  DIR *directory = opendir("/proc/self/fd");
  struct dirent *entry;
  int count = 0;
  if (directory == NULL) {
    return -1;
  }
  while ((entry = readdir(directory)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      count += 1;
    }
  }
  (void)closedir(directory);
  return count;
}

static void test_large_echo(
    tcpip_test_context *test,
    tcpip_linux_l01_trigger trigger) {
  int sockets[2] = {-1, -1};
  int buffer_size = 2048;
  int original_flags;
  int final_flags;
  int before_fds = count_open_fds();
  pthread_t thread;
  peer_context peer;
  tcpip_linux_l01_stats stats;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    TCPIP_FAIL(test, "socketpair failed");
    return;
  }
  (void)setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
  (void)setsockopt(sockets[0], SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
  (void)setsockopt(sockets[1], SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
  (void)setsockopt(sockets[1], SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
  original_flags = fcntl(sockets[0], F_GETFL);
  fill_payload(&peer, TEST_PAYLOAD_SIZE);
  peer.fd = sockets[1];
  if (pthread_create(&thread, NULL, peer_exchange, &peer) != 0) {
    TCPIP_FAIL(test, "pthread_create failed");
    (void)close(sockets[0]);
    (void)close(sockets[1]);
    return;
  }

  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(sockets[0], trigger, peer.length, 5000, &stats),
      TCPIP_LINUX_L01_OK);
  (void)pthread_join(thread, NULL);
  final_flags = fcntl(sockets[0], F_GETFL);

  TCPIP_EXPECT_U32(test, (uint32_t)peer.error, 0U);
  TCPIP_EXPECT_SIZE(test, peer.sent_count, peer.length);
  TCPIP_EXPECT_SIZE(test, peer.received_count, peer.length);
  TCPIP_EXPECT_BYTES(test, peer.received, peer.length, peer.sent, peer.length);
  TCPIP_EXPECT_SIZE(test, (size_t)stats.bytes_read, peer.length);
  TCPIP_EXPECT_SIZE(test, (size_t)stats.bytes_written, peer.length);
  TCPIP_EXPECT_TRUE(test, stats.readiness_events > 1U);
  TCPIP_EXPECT_TRUE(test, stats.read_calls > 1U);
  TCPIP_EXPECT_TRUE(test, stats.write_calls > 1U);
  if (trigger == TCPIP_LINUX_L01_TRIGGER_EDGE) {
    TCPIP_EXPECT_TRUE(test, stats.drain_passes > 0U);
    TCPIP_EXPECT_TRUE(test, stats.read_eagain + stats.write_eagain > 0U);
  }
  TCPIP_EXPECT_U32(test, (uint32_t)final_flags, (uint32_t)original_flags);

  (void)close(sockets[0]);
  (void)close(sockets[1]);
  if (before_fds >= 0) {
    TCPIP_EXPECT_U32(test, (uint32_t)count_open_fds(), (uint32_t)before_fds);
  }
}

static int64_t elapsed_ms(const struct timespec *start, const struct timespec *end) {
  int64_t seconds = (int64_t)(end->tv_sec - start->tv_sec);
  int64_t nanoseconds = (int64_t)(end->tv_nsec - start->tv_nsec);
  return seconds * INT64_C(1000) + nanoseconds / INT64_C(1000000);
}

static void test_timeout(tcpip_test_context *test) {
  int sockets[2];
  struct timespec start;
  struct timespec end;
  tcpip_linux_l01_stats stats;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    TCPIP_FAIL(test, "socketpair failed");
    return;
  }
  (void)clock_gettime(CLOCK_MONOTONIC, &start);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          sockets[0], TCPIP_LINUX_L01_TRIGGER_EDGE, 1U, 80, &stats),
      TCPIP_LINUX_L01_TIMEOUT);
  (void)clock_gettime(CLOCK_MONOTONIC, &end);
  TCPIP_EXPECT_TRUE(test, elapsed_ms(&start, &end) >= INT64_C(50));
  TCPIP_EXPECT_TRUE(test, elapsed_ms(&start, &end) < INT64_C(1000));
  (void)close(sockets[0]);
  (void)close(sockets[1]);
}

static void test_eof(tcpip_test_context *test) {
  int sockets[2];
  tcpip_linux_l01_stats stats;
  static const uint8_t partial[] = {'p', 'a', 'r', 't'};

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    TCPIP_FAIL(test, "socketpair failed");
    return;
  }
  (void)close(sockets[1]);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          sockets[0], TCPIP_LINUX_L01_TRIGGER_LEVEL, 1U, 500, &stats),
      TCPIP_LINUX_L01_EOF);
  (void)close(sockets[0]);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    TCPIP_FAIL(test, "socketpair failed");
    return;
  }
  TCPIP_EXPECT_SIZE(
      test, (size_t)send(sockets[1], partial, sizeof(partial), MSG_NOSIGNAL), sizeof(partial));
  (void)shutdown(sockets[1], SHUT_WR);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          sockets[0], TCPIP_LINUX_L01_TRIGGER_EDGE, 16U, 500, &stats),
      TCPIP_LINUX_L01_TRUNCATED);
  TCPIP_EXPECT_SIZE(test, (size_t)stats.bytes_read, sizeof(partial));
  TCPIP_EXPECT_SIZE(test, (size_t)stats.bytes_written, sizeof(partial));
  (void)close(sockets[0]);
  (void)close(sockets[1]);
}

static void test_validation_and_flags(tcpip_test_context *test) {
  tcpip_linux_l01_stats stats;
  int sockets[2];

  memset(&stats, 0xa5, sizeof(stats));
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          -1, TCPIP_LINUX_L01_TRIGGER_LEVEL, 1U, 1, &stats),
      TCPIP_LINUX_L01_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, (size_t)stats.bytes_read, 0U);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          0, (tcpip_linux_l01_trigger)99, 1U, 1, &stats),
      TCPIP_LINUX_L01_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          0, TCPIP_LINUX_L01_TRIGGER_LEVEL, 1U, -1, &stats),
      TCPIP_LINUX_L01_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          0, TCPIP_LINUX_L01_TRIGGER_LEVEL, 1U, 1, NULL),
      TCPIP_LINUX_L01_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l01_set_nonblocking(-1), TCPIP_LINUX_L01_INVALID_ARGUMENT);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    TCPIP_FAIL(test, "socketpair failed");
    return;
  }
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l01_set_nonblocking(sockets[0]), TCPIP_LINUX_L01_OK);
  TCPIP_EXPECT_TRUE(test, (fcntl(sockets[0], F_GETFL) & O_NONBLOCK) != 0);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l01_echo(
          sockets[0], TCPIP_LINUX_L01_TRIGGER_LEVEL, 0U, 0, &stats),
      TCPIP_LINUX_L01_OK);
  TCPIP_EXPECT_TRUE(test, (fcntl(sockets[0], F_GETFL) & O_NONBLOCK) != 0);
  (void)close(sockets[0]);
  (void)close(sockets[1]);
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "Linux lab 01 epoll event loop");
  test_validation_and_flags(&test);
  test_large_echo(&test, TCPIP_LINUX_L01_TRIGGER_LEVEL);
  test_large_echo(&test, TCPIP_LINUX_L01_TRIGGER_EDGE);
  test_eof(&test);
  test_timeout(&test);
  return tcpip_test_finish(&test);
}
