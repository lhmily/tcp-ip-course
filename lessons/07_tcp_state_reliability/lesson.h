#ifndef TCPIP_L07_LESSON_H
#define TCPIP_L07_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_l07_status {
  TCPIP_L07_OK = 0,
  TCPIP_L07_INVALID_ARGUMENT,
  TCPIP_L07_TRUNCATED,
  TCPIP_L07_MALFORMED,
  TCPIP_L07_CAPACITY,
  TCPIP_L07_TODO
} tcpip_l07_status;

typedef enum tcpip_l07_tcp_state {
  TCPIP_L07_TCP_STATE_CLOSED = 0,
  TCPIP_L07_TCP_STATE_LISTEN,
  TCPIP_L07_TCP_STATE_SYN_SENT,
  TCPIP_L07_TCP_STATE_SYN_RECEIVED,
  TCPIP_L07_TCP_STATE_ESTABLISHED,
  TCPIP_L07_TCP_STATE_FIN_WAIT_1,
  TCPIP_L07_TCP_STATE_FIN_WAIT_2,
  TCPIP_L07_TCP_STATE_CLOSE_WAIT,
  TCPIP_L07_TCP_STATE_LAST_ACK,
  TCPIP_L07_TCP_STATE_TIME_WAIT
} tcpip_l07_tcp_state;

typedef enum tcpip_l07_tcp_event {
  TCPIP_L07_TCP_EVENT_PASSIVE_OPEN = 0,
  TCPIP_L07_TCP_EVENT_ACTIVE_OPEN,
  TCPIP_L07_TCP_EVENT_RECEIVE_SYN,
  TCPIP_L07_TCP_EVENT_RECEIVE_SYN_ACK,
  TCPIP_L07_TCP_EVENT_RECEIVE_ACK,
  TCPIP_L07_TCP_EVENT_APP_CLOSE,
  TCPIP_L07_TCP_EVENT_RECEIVE_FIN,
  TCPIP_L07_TCP_EVENT_RECEIVE_FIN_ACK,
  TCPIP_L07_TCP_EVENT_TIMEOUT
} tcpip_l07_tcp_event;

typedef struct tcpip_l07_reassembly {
  uint32_t initial_seq;
  uint8_t *data;
  uint8_t *present;
  size_t capacity;
  size_t read_offset;
} tcpip_l07_reassembly;

tcpip_l07_status tcpip_l07_transition(
    tcpip_l07_tcp_state state,
    tcpip_l07_tcp_event event,
    tcpip_l07_tcp_state *out_state);

/*
 * Bind a receive window to separate caller-owned arrays. For nonzero capacity,
 * data[0..capacity) and present[0..capacity) must be non-null and must not
 * overlap each other.
 */
tcpip_l07_status tcpip_l07_reassembly_init(
    tcpip_l07_reassembly *ctx,
    uint32_t initial_seq,
    uint8_t *data,
    uint8_t *present,
    size_t capacity);

tcpip_l07_status tcpip_l07_reassembly_push(
    tcpip_l07_reassembly *ctx,
    uint32_t seq,
    const uint8_t *input,
    size_t len,
    size_t *accepted);

/*
 * Copy the entire contiguous unread run. For nonzero out_capacity, the output
 * span must not overlap either complete backing array bound by init.
 */
tcpip_l07_status tcpip_l07_reassembly_read(
    tcpip_l07_reassembly *ctx,
    uint8_t *out,
    size_t out_capacity,
    size_t *produced);

#ifdef __cplusplus
}
#endif

#endif
