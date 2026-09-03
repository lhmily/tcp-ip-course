#include "lesson.h"

#include <limits.h>
#include <string.h>

static int tcpip_l07_state_is_valid(tcpip_l07_tcp_state state) {
  return state >= TCPIP_L07_TCP_STATE_CLOSED && state <= TCPIP_L07_TCP_STATE_TIME_WAIT;
}

static int tcpip_l07_event_is_valid(tcpip_l07_tcp_event event) {
  return event >= TCPIP_L07_TCP_EVENT_PASSIVE_OPEN && event <= TCPIP_L07_TCP_EVENT_TIMEOUT;
}

static int tcpip_l07_reassembly_is_valid(const tcpip_l07_reassembly *ctx) {
  if (ctx == NULL || ctx->capacity > (size_t)UINT32_MAX || ctx->read_offset > ctx->capacity) {
    return 0;
  }
  if (ctx->capacity != 0U && (ctx->data == NULL || ctx->present == NULL)) {
    return 0;
  }
  return 1;
}

tcpip_l07_status tcpip_l07_transition(
    tcpip_l07_tcp_state state,
    tcpip_l07_tcp_event event,
    tcpip_l07_tcp_state *out_state) {
  if (out_state == NULL) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }
  *out_state = TCPIP_L07_TCP_STATE_CLOSED;
  if (!tcpip_l07_state_is_valid(state) || !tcpip_l07_event_is_valid(event)) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }
  return TCPIP_L07_TODO;
}

tcpip_l07_status tcpip_l07_reassembly_init(
    tcpip_l07_reassembly *ctx,
    uint32_t initial_seq,
    uint8_t *data,
    uint8_t *present,
    size_t capacity) {
  if (ctx == NULL) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }

  ctx->initial_seq = 0U;
  ctx->data = NULL;
  ctx->present = NULL;
  ctx->capacity = 0U;
  ctx->read_offset = 0U;
  if (capacity > (size_t)UINT32_MAX || (capacity != 0U && (data == NULL || present == NULL))) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }

  if (capacity != 0U) {
    memset(present, 0, capacity);
  }
  ctx->initial_seq = initial_seq;
  ctx->data = data;
  ctx->present = present;
  ctx->capacity = capacity;
  return TCPIP_L07_TODO;
}

tcpip_l07_status tcpip_l07_reassembly_push(
    tcpip_l07_reassembly *ctx,
    uint32_t seq,
    const uint8_t *input,
    size_t len,
    size_t *accepted) {
  if (accepted == NULL) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }
  *accepted = 0U;
  if (!tcpip_l07_reassembly_is_valid(ctx) || (len != 0U && input == NULL)) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }
  (void)seq;
  return TCPIP_L07_TODO;
}

tcpip_l07_status tcpip_l07_reassembly_read(
    tcpip_l07_reassembly *ctx,
    uint8_t *out,
    size_t out_capacity,
    size_t *produced) {
  if (produced == NULL) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }
  *produced = 0U;
  if (!tcpip_l07_reassembly_is_valid(ctx) || (out_capacity != 0U && out == NULL)) {
    return TCPIP_L07_INVALID_ARGUMENT;
  }
  return TCPIP_L07_TODO;
}
