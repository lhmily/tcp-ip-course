#ifndef TCPIP_LINUX_L01_LAB_H
#define TCPIP_LINUX_L01_LAB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_linux_l01_status {
  TCPIP_LINUX_L01_OK = 0,
  TCPIP_LINUX_L01_INVALID_ARGUMENT = 1,
  TCPIP_LINUX_L01_EOF = 2,
  TCPIP_LINUX_L01_TRUNCATED = 3,
  TCPIP_LINUX_L01_TIMEOUT = 4,
  TCPIP_LINUX_L01_SYSTEM = 5,
  TCPIP_LINUX_L01_TODO = 6
} tcpip_linux_l01_status;

typedef enum tcpip_linux_l01_trigger {
  TCPIP_LINUX_L01_TRIGGER_LEVEL = 0,
  TCPIP_LINUX_L01_TRIGGER_EDGE = 1
} tcpip_linux_l01_trigger;

typedef struct tcpip_linux_l01_stats {
  uint64_t readiness_events;
  uint64_t read_calls;
  uint64_t write_calls;
  uint64_t bytes_read;
  uint64_t bytes_written;
  uint64_t read_eagain;
  uint64_t write_eagain;
  uint64_t drain_passes;
} tcpip_linux_l01_stats;

tcpip_linux_l01_status tcpip_linux_l01_set_nonblocking(int fd);

tcpip_linux_l01_status tcpip_linux_l01_echo(
    int fd,
    tcpip_linux_l01_trigger trigger,
    size_t expected_bytes,
    int timeout_ms,
    tcpip_linux_l01_stats *stats);

#ifdef __cplusplus
}
#endif

#endif
