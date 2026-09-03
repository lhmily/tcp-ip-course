#include "lab.h"

#include "../../lessons/03_ipv4_packets/lesson.h"
#include "../../lessons/06_tcp_segments/lesson.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TCPIP_LINUX_L03_CHUNK 256U
#define TCPIP_LINUX_L03_ETHERNET_HEADER 14U
#define TCPIP_LINUX_L03_IPV4_HEADER 20U
#define TCPIP_LINUX_L03_TCP_HEADER 20U
#define TCPIP_LINUX_L03_IPV4_DF UINT16_C(0x4000)
#define TCPIP_LINUX_L03_MAX_CHUNKS \
  ((TCPIP_LINUX_L03_MAX_PAYLOAD + TCPIP_LINUX_L03_CHUNK - 1U) / TCPIP_LINUX_L03_CHUNK)

typedef struct tcpip_linux_l03_event {
  uint8_t frame[TCPIP_LINUX_L03_MAX_FRAME];
  size_t frame_length;
  size_t chunk_index;
  uint64_t due_ms;
  size_t order;
  int used;
} tcpip_linux_l03_event;

static int tcpip_linux_l03_span_is_valid(const void *span, size_t length) {
  return length == 0U || span != NULL;
}

static uint16_t tcpip_linux_l03_read_be16(const uint8_t *bytes) {
  return (uint16_t)(((uint16_t)bytes[0] << 8U) | (uint16_t)bytes[1]);
}

static void tcpip_linux_l03_write_be16(uint8_t *bytes, uint16_t value) {
  bytes[0] = (uint8_t)(value >> 8U);
  bytes[1] = (uint8_t)value;
}

static uint16_t tcpip_linux_l03_checksum(const uint8_t *bytes, size_t length) {
  uint32_t sum = 0U;
  size_t offset = 0U;

  while (length - offset >= 2U) {
    sum += (uint32_t)tcpip_linux_l03_read_be16(bytes + offset);
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
    offset += 2U;
  }
  if (offset < length) {
    sum += (uint32_t)bytes[offset] << 8U;
  }
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)~sum;
}

static tcpip_linux_l03_status tcpip_linux_l03_validate(
    const tcpip_linux_l03_scenario *scenario,
    tcpip_linux_l03_result *result) {
  size_t index;

  if (result == NULL) {
    return TCPIP_LINUX_L03_INVALID_ARGUMENT;
  }
  memset(result, 0, sizeof(*result));
  result->route_index = SIZE_MAX;
  result->final_state = TCPIP_L07_TCP_STATE_CLOSED;
  if (scenario == NULL ||
      !tcpip_linux_l03_span_is_valid(scenario->payload, scenario->payload_length) ||
      !tcpip_linux_l03_span_is_valid(scenario->output, scenario->output_capacity) ||
      !tcpip_linux_l03_span_is_valid(scenario->routes, scenario->route_count) ||
      !tcpip_linux_l03_span_is_valid(scenario->faults, scenario->fault_count)) {
    return TCPIP_LINUX_L03_INVALID_ARGUMENT;
  }
  if (scenario->payload_length == 0U ||
      scenario->payload_length > TCPIP_LINUX_L03_MAX_PAYLOAD ||
      scenario->fault_count > TCPIP_LINUX_L03_MAX_FAULT_ACTIONS ||
      scenario->max_attempts == 0U || scenario->initial_rto_ms == 0U ||
      scenario->source_port == 0U || scenario->destination_port == 0U) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  if (scenario->output_capacity < scenario->payload_length) {
    return TCPIP_LINUX_L03_CAPACITY;
  }
  for (index = 0U; index < scenario->fault_count; index += 1U) {
    if (scenario->faults[index].action < TCPIP_LINUX_L03_DELIVER ||
        scenario->faults[index].action > TCPIP_LINUX_L03_REORDER) {
      return TCPIP_LINUX_L03_MALFORMED;
    }
  }
  return TCPIP_LINUX_L03_OK;
}

static tcpip_linux_l03_status tcpip_linux_l03_build_frame(
    const tcpip_linux_l03_scenario *scenario,
    size_t chunk_index,
    uint8_t *frame,
    size_t *frame_length) {
  static const uint8_t destination_mac[6] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
  static const uint8_t source_mac[6] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};
  tcpip_l06_segment_fields fields;
  tcpip_l03_ipv4_header_fields ipv4_fields;
  tcpip_l03_ipv4_packet parsed_ipv4;
  uint8_t *ipv4 = frame + TCPIP_LINUX_L03_ETHERNET_HEADER;
  uint8_t *tcp = ipv4 + TCPIP_LINUX_L03_IPV4_HEADER;
  const size_t payload_offset = chunk_index * TCPIP_LINUX_L03_CHUNK;
  const size_t remaining = scenario->payload_length - payload_offset;
  const size_t payload_length = remaining < TCPIP_LINUX_L03_CHUNK ? remaining : TCPIP_LINUX_L03_CHUNK;
  size_t tcp_length = 0U;
  size_t header_length = 0U;
  size_t ipv4_length;

  memset(frame, 0, TCPIP_LINUX_L03_MAX_FRAME);
  memcpy(frame, destination_mac, sizeof(destination_mac));
  memcpy(frame + 6U, source_mac, sizeof(source_mac));
  tcpip_linux_l03_write_be16(frame + 12U, UINT16_C(0x0800));

  memset(&fields, 0, sizeof(fields));
  fields.source_ipv4 = scenario->source_ipv4;
  fields.source_ipv4_length = sizeof(scenario->source_ipv4);
  fields.destination_ipv4 = scenario->destination_ipv4;
  fields.destination_ipv4_length = sizeof(scenario->destination_ipv4);
  fields.source_port = scenario->source_port;
  fields.destination_port = scenario->destination_port;
  fields.sequence_number = scenario->sender_initial_seq + (uint32_t)payload_offset;
  fields.acknowledgment_number = scenario->receiver_initial_seq;
  fields.flags = TCPIP_L06_FLAG_ACK | TCPIP_L06_FLAG_PSH;
  fields.window = UINT16_C(4096);
  if (tcpip_l06_build_segment(
          &fields,
          scenario->payload + payload_offset,
          payload_length,
          tcp,
          TCPIP_LINUX_L03_MAX_FRAME - TCPIP_LINUX_L03_ETHERNET_HEADER -
              TCPIP_LINUX_L03_IPV4_HEADER,
          &tcp_length) != TCPIP_L06_OK) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  memset(&ipv4_fields, 0, sizeof(ipv4_fields));
  ipv4_fields.identification = (uint16_t)chunk_index;
  ipv4_fields.flags_fragment = TCPIP_LINUX_L03_IPV4_DF;
  ipv4_fields.ttl = UINT8_C(64);
  ipv4_fields.protocol = UINT8_C(6);
  memcpy(ipv4_fields.source, scenario->source_ipv4, sizeof(ipv4_fields.source));
  memcpy(ipv4_fields.destination, scenario->destination_ipv4, sizeof(ipv4_fields.destination));
  if (tcpip_l03_build_header(
          &ipv4_fields, ipv4, TCPIP_LINUX_L03_IPV4_HEADER, &header_length) != TCPIP_L03_OK ||
      header_length != TCPIP_LINUX_L03_IPV4_HEADER) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  ipv4_length = TCPIP_LINUX_L03_IPV4_HEADER + tcp_length;
  tcpip_linux_l03_write_be16(ipv4 + 2U, (uint16_t)ipv4_length);
  ipv4[10U] = 0U;
  ipv4[11U] = 0U;
  tcpip_linux_l03_write_be16(
      ipv4 + 10U, tcpip_linux_l03_checksum(ipv4, TCPIP_LINUX_L03_IPV4_HEADER));
  if (tcpip_l03_parse_ipv4(ipv4, ipv4_length, &parsed_ipv4) != TCPIP_L03_OK ||
      parsed_ipv4.payload_length != tcp_length) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  *frame_length = TCPIP_LINUX_L03_ETHERNET_HEADER + ipv4_length;
  return TCPIP_LINUX_L03_OK;
}

static tcpip_linux_l03_status tcpip_linux_l03_enqueue(
    tcpip_linux_l03_event events[TCPIP_LINUX_L03_MAX_EVENTS],
    const tcpip_linux_l03_scenario *scenario,
    size_t chunk_index,
    uint64_t due_ms,
    size_t order) {
  size_t index;

  for (index = 0U; index < TCPIP_LINUX_L03_MAX_EVENTS; index += 1U) {
    if (events[index].used == 0) {
      tcpip_linux_l03_status status = tcpip_linux_l03_build_frame(
          scenario, chunk_index, events[index].frame, &events[index].frame_length);
      if (status != TCPIP_LINUX_L03_OK) {
        return status;
      }
      events[index].chunk_index = chunk_index;
      events[index].due_ms = due_ms;
      events[index].order = order;
      events[index].used = 1;
      return TCPIP_LINUX_L03_OK;
    }
  }
  return TCPIP_LINUX_L03_CAPACITY;
}

static size_t tcpip_linux_l03_next_event(
    const tcpip_linux_l03_event events[TCPIP_LINUX_L03_MAX_EVENTS],
    uint64_t deadline) {
  size_t best = SIZE_MAX;
  size_t index;

  for (index = 0U; index < TCPIP_LINUX_L03_MAX_EVENTS; index += 1U) {
    if (events[index].used == 0 || events[index].due_ms > deadline) {
      continue;
    }
    if (best == SIZE_MAX || events[index].due_ms < events[best].due_ms ||
        (events[index].due_ms == events[best].due_ms && events[index].order < events[best].order)) {
      best = index;
    }
  }
  return best;
}

static tcpip_linux_l03_status tcpip_linux_l03_deliver(
    int sender_fd,
    int receiver_fd,
    tcpip_linux_l03_event *event,
    const tcpip_linux_l03_scenario *scenario,
    tcpip_l07_reassembly *reassembly,
    uint8_t acknowledged[TCPIP_LINUX_L03_MAX_CHUNKS],
    tcpip_linux_l03_result *result) {
  uint8_t received[TCPIP_LINUX_L03_MAX_FRAME];
  tcpip_l03_ipv4_packet ipv4;
  tcpip_l06_segment tcp;
  uint16_t transport_checksum = 1U;
  ssize_t sent;
  ssize_t received_length;
  size_t accepted = 0U;
  const uint8_t *ipv4_bytes;
  const uint8_t *tcp_bytes;

  sent = send(sender_fd, event->frame, event->frame_length, 0);
  if (sent < 0 || (size_t)sent != event->frame_length) {
    return TCPIP_LINUX_L03_SYSTEM_ERROR;
  }
  received_length = recv(receiver_fd, received, sizeof(received), 0);
  if (received_length < 0 || (size_t)received_length != event->frame_length) {
    return TCPIP_LINUX_L03_SYSTEM_ERROR;
  }
  ipv4_bytes = received + TCPIP_LINUX_L03_ETHERNET_HEADER;
  if (tcpip_l03_parse_ipv4(
          ipv4_bytes,
          (size_t)received_length - TCPIP_LINUX_L03_ETHERNET_HEADER,
          &ipv4) != TCPIP_L03_OK) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  tcp_bytes = ipv4_bytes + ipv4.payload_offset;
  if (tcpip_l06_parse_segment(tcp_bytes, ipv4.payload_length, &tcp) != TCPIP_L06_OK ||
      tcpip_l06_ipv4_checksum(
          scenario->source_ipv4,
          sizeof(scenario->source_ipv4),
          scenario->destination_ipv4,
          sizeof(scenario->destination_ipv4),
          tcp_bytes,
          ipv4.payload_length,
          &transport_checksum) != TCPIP_L06_OK ||
      transport_checksum != 0U) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  {
    tcpip_l07_status reassembly_status = tcpip_l07_reassembly_push(
        reassembly,
        tcp.sequence_number,
        tcp_bytes + tcp.payload_offset,
        tcp.payload_length,
        &accepted);
    if (reassembly_status == TCPIP_L07_CAPACITY &&
        acknowledged[event->chunk_index] != 0U) {
      accepted = 0U;
    } else if (reassembly_status != TCPIP_L07_OK) {
      return TCPIP_LINUX_L03_MALFORMED;
    }
  }
  (void)accepted;
  acknowledged[event->chunk_index] = 1U;
  memset(&result->final_frame_report, 0, sizeof(result->final_frame_report));
  if (tcpip_l12_diagnose_frame(received, (size_t)received_length, &result->final_frame_report) !=
      TCPIP_L12_OK) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  if (tcpip_l12_format_report(
          &result->final_frame_report,
          result->final_frame_diagnostic,
          sizeof(result->final_frame_diagnostic),
          &result->final_frame_diagnostic_length) != TCPIP_L12_OK) {
    return TCPIP_LINUX_L03_CAPACITY;
  }
  event->used = 0;
  return TCPIP_LINUX_L03_OK;
}

static int tcpip_linux_l03_all_acknowledged(
    const uint8_t acknowledged[TCPIP_LINUX_L03_MAX_CHUNKS], size_t chunk_count) {
  size_t index;
  for (index = 0U; index < chunk_count; index += 1U) {
    if (acknowledged[index] == 0U) {
      return 0;
    }
  }
  return 1;
}

tcpip_linux_l03_status tcpip_linux_l03_run(
    const tcpip_linux_l03_scenario *scenario,
    tcpip_linux_l03_result *result) {
  tcpip_linux_l03_event events[TCPIP_LINUX_L03_MAX_EVENTS];
  uint8_t reassembly_data[TCPIP_LINUX_L03_MAX_PAYLOAD];
  uint8_t reassembly_present[TCPIP_LINUX_L03_MAX_PAYLOAD];
  uint8_t acknowledged[TCPIP_LINUX_L03_MAX_CHUNKS];
  tcpip_l07_reassembly reassembly;
  tcpip_l07_tcp_state state;
  tcpip_linux_l03_status status;
  size_t chunk_count;
  size_t fault_index = 0U;
  size_t event_order = 0U;
  uint64_t now = 0U;
  uint64_t rto;
  int sockets[2] = {-1, -1};
  int flags;
  size_t attempt;

  status = tcpip_linux_l03_validate(scenario, result);
  if (status != TCPIP_LINUX_L03_OK) {
    return status;
  }
  if (tcpip_l11_longest_prefix(
          scenario->routes, scenario->route_count, scenario->destination_ipv4,
          &result->route_index) != TCPIP_L11_OK) {
    return TCPIP_LINUX_L03_ROUTE_NOT_FOUND;
  }
  if (tcpip_l07_transition(
          TCPIP_L07_TCP_STATE_CLOSED, TCPIP_L07_TCP_EVENT_ACTIVE_OPEN, &state) != TCPIP_L07_OK ||
      tcpip_l07_transition(
          state, TCPIP_L07_TCP_EVENT_RECEIVE_SYN_ACK, &state) != TCPIP_L07_OK) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  if (tcpip_l07_reassembly_init(
          &reassembly,
          scenario->sender_initial_seq,
          reassembly_data,
          reassembly_present,
          scenario->payload_length) != TCPIP_L07_OK) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  memset(events, 0, sizeof(events));
  memset(acknowledged, 0, sizeof(acknowledged));
  chunk_count = (scenario->payload_length + TCPIP_LINUX_L03_CHUNK - 1U) /
                TCPIP_LINUX_L03_CHUNK;
  rto = scenario->initial_rto_ms;

  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets) != 0) {
    return TCPIP_LINUX_L03_SYSTEM_ERROR;
  }
  flags = fcntl(sockets[0], F_GETFL, 0);
  if (flags < 0 || fcntl(sockets[0], F_SETFL, flags | O_NONBLOCK) != 0) {
    status = TCPIP_LINUX_L03_SYSTEM_ERROR;
    goto cleanup;
  }
  flags = fcntl(sockets[1], F_GETFL, 0);
  if (flags < 0 || fcntl(sockets[1], F_SETFL, flags | O_NONBLOCK) != 0) {
    status = TCPIP_LINUX_L03_SYSTEM_ERROR;
    goto cleanup;
  }

  for (attempt = 0U; attempt < scenario->max_attempts; attempt += 1U) {
    uint64_t deadline = UINT64_MAX - now < rto ? UINT64_MAX : now + rto;
    size_t chunk;
    result->attempts = attempt + 1U;
    for (chunk = 0U; chunk < TCPIP_LINUX_L03_MAX_EVENTS; chunk += 1U) {
      if (events[chunk].used != 0 && acknowledged[events[chunk].chunk_index] != 0U) {
        events[chunk].used = 0;
      }
    }


    for (chunk = 0U; chunk < chunk_count; chunk += 1U) {
      tcpip_linux_l03_fault fault = {TCPIP_LINUX_L03_DELIVER, 0U};
      uint64_t due = now;
      if (acknowledged[chunk] != 0U) {
        continue;
      }
      if (fault_index < scenario->fault_count) {
        fault = scenario->faults[fault_index];
      }
      fault_index += 1U;
      if (fault.action == TCPIP_LINUX_L03_DROP) {
        continue;
      }
      if (fault.action == TCPIP_LINUX_L03_DELAY) {
        if (UINT64_MAX - due < fault.delay_ms) {
          status = TCPIP_LINUX_L03_CAPACITY;
          goto cleanup;
        }
        due += fault.delay_ms;
      } else if (fault.action == TCPIP_LINUX_L03_REORDER) {
        uint64_t offset = rto > 1U ? rto / 2U : 1U;
        if (UINT64_MAX - due < offset) {
          status = TCPIP_LINUX_L03_CAPACITY;
          goto cleanup;
        }
        due += offset;
      }
      status = tcpip_linux_l03_enqueue(events, scenario, chunk, due, event_order);
      event_order += 1U;
      if (status != TCPIP_LINUX_L03_OK) {
        goto cleanup;
      }
    }

    for (;;) {
      size_t event_index = tcpip_linux_l03_next_event(events, deadline);
      if (event_index == SIZE_MAX) {
        break;
      }
      now = events[event_index].due_ms;
      status = tcpip_linux_l03_deliver(
          sockets[0], sockets[1], &events[event_index], scenario, &reassembly,
          acknowledged, result);
      if (status != TCPIP_LINUX_L03_OK) {
        goto cleanup;
      }
    }
    if (tcpip_linux_l03_all_acknowledged(acknowledged, chunk_count) != 0) {
      size_t produced = 0U;
      if (tcpip_l07_reassembly_read(
              &reassembly, scenario->output, scenario->output_capacity, &produced) != TCPIP_L07_OK ||
          produced != scenario->payload_length) {
        status = TCPIP_LINUX_L03_MALFORMED;
        goto cleanup;
      }
      result->reassembled_length = produced;
      result->virtual_elapsed_ms = now;
      result->final_state = state;
      status = TCPIP_LINUX_L03_OK;
      goto cleanup;
    }
    now = deadline;
    if (attempt + 1U < scenario->max_attempts) {
      result->retransmits += 1U;
    }
    if (rto <= UINT64_MAX / 2U) {
      rto *= 2U;
    } else {
      rto = UINT64_MAX;
    }
  }

  result->virtual_elapsed_ms = now;
  result->final_state = TCPIP_L07_TCP_STATE_CLOSED;
  status = TCPIP_LINUX_L03_RETRY_EXHAUSTED;

cleanup:
  if (sockets[0] >= 0) {
    (void)close(sockets[0]);
  }
  if (sockets[1] >= 0) {
    (void)close(sockets[1]);
  }
  return status;
}
