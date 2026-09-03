#ifndef TCPIP_L08_LESSON_H
#define TCPIP_L08_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The status value is independent of errno; TCPIP_L08_SYSTEM means a POSIX call failed. */
typedef enum tcpip_l08_status {
  TCPIP_L08_OK = 0,
  TCPIP_L08_INVALID_ARGUMENT,
  TCPIP_L08_TRUNCATED,
  TCPIP_L08_MALFORMED,
  TCPIP_L08_CAPACITY,
  TCPIP_L08_TODO,
  TCPIP_L08_EOF,
  TCPIP_L08_TIMEOUT,
  TCPIP_L08_SYSTEM
} tcpip_l08_status;

/* Maximum payload accepted by tcpip_l08_serve_one. */
#define TCPIP_L08_SERVER_FRAME_CAPACITY ((size_t)65536U)

/*
 * Transfer exactly len bytes before one CLOCK_MONOTONIC deadline expires.
 * timeout_ms must be nonnegative. data/out may be NULL only when len is zero.
 * EOF before any requested byte returns EOF; EOF after progress returns TRUNCATED.
 */
tcpip_l08_status tcpip_l08_send_all(
    int fd, const uint8_t *data, size_t len, int timeout_ms);
tcpip_l08_status tcpip_l08_recv_exact(
    int fd, uint8_t *out, size_t len, int timeout_ms);

/* Send a four-byte big-endian uint32 length followed by len payload bytes. */
tcpip_l08_status tcpip_l08_send_frame(
    int fd, const uint8_t *data, size_t len, int timeout_ms);

/*
 * Receive one length-prefixed frame. *out_len is initialized to zero.
 * A length greater than cap returns CAPACITY without consuming payload bytes.
 */
tcpip_l08_status tcpip_l08_recv_frame(
    int fd, uint8_t *out, size_t cap, size_t *out_len, int timeout_ms);

/*
 * Wait for one connection on listen_fd, accept it, receive one frame no larger
 * than TCPIP_L08_SERVER_FRAME_CAPACITY, echo that frame, close the accepted fd,
 * and return the receive/send/close status. One absolute deadline covers accept,
 * receive, and send; the listening fd remains open with its original file status
 * flags restored and is never closed here.
 */
tcpip_l08_status tcpip_l08_serve_one(int listen_fd, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
