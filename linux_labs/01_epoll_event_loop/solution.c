#include "lab.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TCPIP_LINUX_L01_BUFFER_SIZE 16384U

static void tcpip_linux_l01_clear_stats(tcpip_linux_l01_stats *stats) {
  if (stats != NULL) {
    memset(stats, 0, sizeof(*stats));
  }
}

static int tcpip_linux_l01_get_flags(int fd, int *flags) {
  int result;
  do {
    result = fcntl(fd, F_GETFL);
  } while (result < 0 && errno == EINTR);
  if (result < 0) {
    return -1;
  }
  *flags = result;
  return 0;
}

static int tcpip_linux_l01_set_flags(int fd, int flags) {
  int result;
  do {
    result = fcntl(fd, F_SETFL, flags);
  } while (result < 0 && errno == EINTR);
  return result;
}

tcpip_linux_l01_status tcpip_linux_l01_set_nonblocking(int fd) {
  int flags;

  if (fd < 0) {
    return TCPIP_LINUX_L01_INVALID_ARGUMENT;
  }
  if (tcpip_linux_l01_get_flags(fd, &flags) < 0) {
    return TCPIP_LINUX_L01_SYSTEM;
  }
  if ((flags & O_NONBLOCK) == 0 &&
      tcpip_linux_l01_set_flags(fd, flags | O_NONBLOCK) < 0) {
    return TCPIP_LINUX_L01_SYSTEM;
  }
  return TCPIP_LINUX_L01_OK;
}

static int tcpip_linux_l01_compare_time(
    const struct timespec *left,
    const struct timespec *right) {
  if (left->tv_sec != right->tv_sec) {
    return left->tv_sec < right->tv_sec ? -1 : 1;
  }
  if (left->tv_nsec != right->tv_nsec) {
    return left->tv_nsec < right->tv_nsec ? -1 : 1;
  }
  return 0;
}

static struct timespec tcpip_linux_l01_deadline(
    const struct timespec *now,
    int timeout_ms) {
  struct timespec result = *now;
  result.tv_sec += (time_t)(timeout_ms / 1000);
  result.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
  if (result.tv_nsec >= 1000000000L) {
    result.tv_sec += (time_t)1;
    result.tv_nsec -= 1000000000L;
  }
  return result;
}

static int tcpip_linux_l01_remaining_ms(const struct timespec *deadline) {
  struct timespec now;
  time_t seconds;
  long nanoseconds;
  int64_t milliseconds;

  if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
    return -1;
  }
  if (tcpip_linux_l01_compare_time(&now, deadline) >= 0) {
    return 0;
  }
  seconds = deadline->tv_sec - now.tv_sec;
  nanoseconds = deadline->tv_nsec - now.tv_nsec;
  if (nanoseconds < 0) {
    seconds -= (time_t)1;
    nanoseconds += 1000000000L;
  }
  if (seconds > (time_t)(INT_MAX / 1000)) {
    return INT_MAX;
  }
  milliseconds = (int64_t)seconds * INT64_C(1000) +
                 ((int64_t)nanoseconds + INT64_C(999999)) / INT64_C(1000000);
  if (milliseconds > INT_MAX) {
    return INT_MAX;
  }
  return (int)milliseconds;
}

static int tcpip_linux_l01_socket_error(int fd) {
  int socket_error = 0;
  socklen_t length = (socklen_t)sizeof(socket_error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &length) < 0) {
    return -1;
  }
  if (socket_error != 0) {
    errno = socket_error;
    return -1;
  }
  return 0;
}

static int tcpip_linux_l01_update_interest(
    int epoll_fd,
    int fd,
    tcpip_linux_l01_trigger trigger,
    int wants_write) {
  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN | EPOLLRDHUP;
  if (wants_write) {
    event.events |= EPOLLOUT;
  }
  if (trigger == TCPIP_LINUX_L01_TRIGGER_EDGE) {
    event.events |= EPOLLET;
  }
  event.data.fd = fd;
  return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

tcpip_linux_l01_status tcpip_linux_l01_echo(
    int fd,
    tcpip_linux_l01_trigger trigger,
    size_t expected_bytes,
    int timeout_ms,
    tcpip_linux_l01_stats *stats) {
  uint8_t buffer[TCPIP_LINUX_L01_BUFFER_SIZE];
  size_t pending_offset = 0U;
  size_t pending_length = 0U;
  size_t total_read = 0U;
  size_t total_written = 0U;
  int original_flags = 0;
  int epoll_fd = -1;
  int flags_changed = 0;
  int eof_seen = 0;
  struct timespec now;
  struct timespec deadline;
  tcpip_linux_l01_status status = TCPIP_LINUX_L01_SYSTEM;

  tcpip_linux_l01_clear_stats(stats);
  if (fd < 0 || stats == NULL || timeout_ms < 0 ||
      (trigger != TCPIP_LINUX_L01_TRIGGER_LEVEL &&
       trigger != TCPIP_LINUX_L01_TRIGGER_EDGE)) {
    return TCPIP_LINUX_L01_INVALID_ARGUMENT;
  }
  if (tcpip_linux_l01_get_flags(fd, &original_flags) < 0) {
    return TCPIP_LINUX_L01_SYSTEM;
  }
  if (expected_bytes == 0U) {
    return TCPIP_LINUX_L01_OK;
  }
  if ((original_flags & O_NONBLOCK) == 0) {
    if (tcpip_linux_l01_set_flags(fd, original_flags | O_NONBLOCK) < 0) {
      return TCPIP_LINUX_L01_SYSTEM;
    }
    flags_changed = 1;
  }
  if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
    goto cleanup;
  }
  deadline = tcpip_linux_l01_deadline(&now, timeout_ms);

  epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    goto cleanup;
  }
  {
    struct epoll_event registration;
    memset(&registration, 0, sizeof(registration));
    registration.events = EPOLLIN | EPOLLRDHUP;
    if (trigger == TCPIP_LINUX_L01_TRIGGER_EDGE) {
      registration.events |= EPOLLET;
    }
    registration.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &registration) < 0) {
      goto cleanup;
    }
  }

  for (;;) {
    struct epoll_event event;
    int wait_ms = tcpip_linux_l01_remaining_ms(&deadline);
    int ready;
    int read_budget;
    int write_budget;

    if (wait_ms < 0) {
      goto cleanup;
    }
    for (;;) {
      ready = epoll_wait(epoll_fd, &event, 1, wait_ms);
      if (ready >= 0 || errno != EINTR) {
        break;
      }
      wait_ms = tcpip_linux_l01_remaining_ms(&deadline);
      if (wait_ms < 0) {
        goto cleanup;
      }
    }
    if (ready < 0) {
      goto cleanup;
    }
    if (ready == 0) {
      status = TCPIP_LINUX_L01_TIMEOUT;
      goto cleanup;
    }
    stats->readiness_events += 1U;

    if ((event.events & EPOLLERR) != 0U &&
        tcpip_linux_l01_socket_error(fd) < 0) {
      goto cleanup;
    }

    read_budget = trigger == TCPIP_LINUX_L01_TRIGGER_EDGE ? INT_MAX : 1;
    write_budget = trigger == TCPIP_LINUX_L01_TRIGGER_EDGE ? INT_MAX : 1;
    for (;;) {
      int progressed = 0;

      if (pending_length != 0U && write_budget > 0) {
        ssize_t sent;
        stats->write_calls += 1U;
        sent = send(
            fd,
            buffer + pending_offset,
            pending_length,
            MSG_NOSIGNAL);
        write_budget -= 1;
        if (sent > 0) {
          size_t amount = (size_t)sent;
          pending_offset += amount;
          pending_length -= amount;
          total_written += amount;
          stats->bytes_written += (uint64_t)amount;
          progressed = 1;
          if (pending_length == 0U) {
            pending_offset = 0U;
          }
        } else if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
          stats->write_eagain += 1U;
        } else if (sent < 0 && errno == EINTR) {
          write_budget += 1;
          continue;
        } else {
          goto cleanup;
        }
      }

      if (!eof_seen && total_read < expected_bytes && read_budget > 0 &&
          pending_length < sizeof(buffer)) {
        size_t capacity;
        ssize_t received;

        if (pending_offset != 0U &&
            pending_offset + pending_length == sizeof(buffer)) {
          memmove(buffer, buffer + pending_offset, pending_length);
          pending_offset = 0U;
        }
        capacity = sizeof(buffer) - (pending_offset + pending_length);
        if (capacity > expected_bytes - total_read) {
          capacity = expected_bytes - total_read;
        }
        stats->read_calls += 1U;
        received = recv(
            fd,
            buffer + pending_offset + pending_length,
            capacity,
            0);
        read_budget -= 1;
        if (received > 0) {
          size_t amount = (size_t)received;
          pending_length += amount;
          total_read += amount;
          stats->bytes_read += (uint64_t)amount;
          progressed = 1;
        } else if (received == 0) {
          eof_seen = 1;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
          stats->read_eagain += 1U;
        } else if (errno == EINTR) {
          read_budget += 1;
          continue;
        } else {
          goto cleanup;
        }
      }

      if (trigger == TCPIP_LINUX_L01_TRIGGER_EDGE) {
        stats->drain_passes += 1U;
      }
      if (total_written == expected_bytes) {
        status = TCPIP_LINUX_L01_OK;
        goto cleanup;
      }
      if (eof_seen && pending_length == 0U) {
        status = total_read == 0U ? TCPIP_LINUX_L01_EOF : TCPIP_LINUX_L01_TRUNCATED;
        goto cleanup;
      }
      if (trigger == TCPIP_LINUX_L01_TRIGGER_LEVEL || !progressed) {
        break;
      }
    }

    if (tcpip_linux_l01_update_interest(
            epoll_fd, fd, trigger, pending_length != 0U) < 0) {
      goto cleanup;
    }
  }

cleanup:
  if (epoll_fd >= 0) {
    (void)close(epoll_fd);
  }
  if (flags_changed && tcpip_linux_l01_set_flags(fd, original_flags) < 0) {
    status = TCPIP_LINUX_L01_SYSTEM;
  }
  return status;
}
