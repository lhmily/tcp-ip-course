#include "lab.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

static void tcpip_linux_l01_clear_stats(tcpip_linux_l01_stats *stats) {
  if (stats != NULL) {
    memset(stats, 0, sizeof(*stats));
  }
}

tcpip_linux_l01_status tcpip_linux_l01_set_nonblocking(int fd) {
  int flags;

  if (fd < 0) {
    return TCPIP_LINUX_L01_INVALID_ARGUMENT;
  }
  do {
    flags = fcntl(fd, F_GETFL);
  } while (flags < 0 && errno == EINTR);
  if (flags < 0) {
    return TCPIP_LINUX_L01_SYSTEM;
  }
  if ((flags & O_NONBLOCK) != 0) {
    return TCPIP_LINUX_L01_OK;
  }
  while (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    if (errno != EINTR) {
      return TCPIP_LINUX_L01_SYSTEM;
    }
  }
  return TCPIP_LINUX_L01_OK;
}

tcpip_linux_l01_status tcpip_linux_l01_echo(
    int fd,
    tcpip_linux_l01_trigger trigger,
    size_t expected_bytes,
    int timeout_ms,
    tcpip_linux_l01_stats *stats) {
  tcpip_linux_l01_clear_stats(stats);
  if (fd < 0 || stats == NULL || timeout_ms < 0 ||
      (trigger != TCPIP_LINUX_L01_TRIGGER_LEVEL &&
       trigger != TCPIP_LINUX_L01_TRIGGER_EDGE)) {
    return TCPIP_LINUX_L01_INVALID_ARGUMENT;
  }
  (void)expected_bytes;
  return TCPIP_LINUX_L01_TODO;
}
