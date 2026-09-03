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

  switch (state) {
    case TCPIP_L07_TCP_STATE_CLOSED:
      if (event == TCPIP_L07_TCP_EVENT_PASSIVE_OPEN) {
        *out_state = TCPIP_L07_TCP_STATE_LISTEN;
        return TCPIP_L07_OK;
      }
      if (event == TCPIP_L07_TCP_EVENT_ACTIVE_OPEN) {
        *out_state = TCPIP_L07_TCP_STATE_SYN_SENT;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_LISTEN:
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_SYN) {
        *out_state = TCPIP_L07_TCP_STATE_SYN_RECEIVED;
        return TCPIP_L07_OK;
      }
      if (event == TCPIP_L07_TCP_EVENT_APP_CLOSE) {
        *out_state = TCPIP_L07_TCP_STATE_CLOSED;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_SYN_SENT:
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_SYN) {
        *out_state = TCPIP_L07_TCP_STATE_SYN_RECEIVED;
        return TCPIP_L07_OK;
      }
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_SYN_ACK) {
        *out_state = TCPIP_L07_TCP_STATE_ESTABLISHED;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_SYN_RECEIVED:
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_ACK) {
        *out_state = TCPIP_L07_TCP_STATE_ESTABLISHED;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_ESTABLISHED:
      if (event == TCPIP_L07_TCP_EVENT_APP_CLOSE) {
        *out_state = TCPIP_L07_TCP_STATE_FIN_WAIT_1;
        return TCPIP_L07_OK;
      }
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_FIN) {
        *out_state = TCPIP_L07_TCP_STATE_CLOSE_WAIT;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_FIN_WAIT_1:
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_ACK) {
        *out_state = TCPIP_L07_TCP_STATE_FIN_WAIT_2;
        return TCPIP_L07_OK;
      }
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_FIN_ACK) {
        *out_state = TCPIP_L07_TCP_STATE_TIME_WAIT;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_FIN_WAIT_2:
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_FIN) {
        *out_state = TCPIP_L07_TCP_STATE_TIME_WAIT;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_CLOSE_WAIT:
      if (event == TCPIP_L07_TCP_EVENT_APP_CLOSE) {
        *out_state = TCPIP_L07_TCP_STATE_LAST_ACK;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_LAST_ACK:
      if (event == TCPIP_L07_TCP_EVENT_RECEIVE_ACK) {
        *out_state = TCPIP_L07_TCP_STATE_CLOSED;
        return TCPIP_L07_OK;
      }
      break;
    case TCPIP_L07_TCP_STATE_TIME_WAIT:
      if (event == TCPIP_L07_TCP_EVENT_TIMEOUT) {
        *out_state = TCPIP_L07_TCP_STATE_CLOSED;
        return TCPIP_L07_OK;
      }
      break;
    default:
      return TCPIP_L07_INVALID_ARGUMENT;
  }

  return TCPIP_L07_MALFORMED;
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
  return TCPIP_L07_OK;
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
  if (len == 0U) {
    return TCPIP_L07_OK;
  }

  const uint32_t offset_u32 = seq - ctx->initial_seq;
  if ((uintmax_t)offset_u32 > (uintmax_t)ctx->capacity) {
    return TCPIP_L07_CAPACITY;
  }
  const size_t offset = (size_t)offset_u32;
  if (len > ctx->capacity - offset) {
    return TCPIP_L07_CAPACITY;
  }

  size_t new_bytes = 0U;
  for (size_t index = 0U; index < len; index += 1U) {
    const size_t destination = offset + index;
    if (ctx->present[destination] != 0U) {
      if (ctx->data[destination] != input[index]) {
        return TCPIP_L07_MALFORMED;
      }
    } else {
      new_bytes += 1U;
    }
  }

  memmove(ctx->data + offset, input, len);
  memset(ctx->present + offset, 1, len);
  *accepted = new_bytes;
  return TCPIP_L07_OK;
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

  size_t available = 0U;
  while (ctx->read_offset + available < ctx->capacity &&
         ctx->present[ctx->read_offset + available] != 0U) {
    available += 1U;
  }
  if (available > out_capacity) {
    return TCPIP_L07_TRUNCATED;
  }
  if (available != 0U) {
    memmove(out, ctx->data + ctx->read_offset, available);
    ctx->read_offset += available;
  }
  *produced = available;
  return TCPIP_L07_OK;
}
