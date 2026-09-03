#include "lab.h"

#include <string.h>

static int tcpip_linux_l03_span_is_valid(const void *span, size_t count, size_t element_size) {
  return count == 0U ||
         (span != NULL && element_size != 0U && count <= SIZE_MAX / element_size);
}

static int tcpip_linux_l03_spans_overlap(
    const void *first,
    size_t first_count,
    size_t first_element_size,
    const void *second,
    size_t second_count,
    size_t second_element_size) {
  uintptr_t first_start;
  uintptr_t second_start;
  size_t first_length;
  size_t second_length;

  if (first_count == 0U || second_count == 0U) {
    return 0;
  }
  if (!tcpip_linux_l03_span_is_valid(first, first_count, first_element_size) ||
      !tcpip_linux_l03_span_is_valid(second, second_count, second_element_size)) {
    return 1;
  }
  first_length = first_count * first_element_size;
  second_length = second_count * second_element_size;
  first_start = (uintptr_t)first;
  second_start = (uintptr_t)second;
  if (first_length > UINTPTR_MAX - first_start ||
      second_length > UINTPTR_MAX - second_start) {
    return 1;
  }
  return first_start < second_start + second_length &&
         second_start < first_start + first_length;
}

tcpip_linux_l03_status tcpip_linux_l03_run(
    const tcpip_linux_l03_scenario *scenario,
    tcpip_linux_l03_result *result) {
  if (result == NULL) {
    return TCPIP_LINUX_L03_INVALID_ARGUMENT;
  }
  if (scenario != NULL &&
      tcpip_linux_l03_spans_overlap(
          scenario, 1U, sizeof(*scenario), result, 1U, sizeof(*result))) {
    memset(result, 0, sizeof(*result));
    result->route_index = SIZE_MAX;
    result->final_state = TCPIP_L07_TCP_STATE_CLOSED;
    return TCPIP_LINUX_L03_INVALID_ARGUMENT;
  }
  memset(result, 0, sizeof(*result));
  result->route_index = SIZE_MAX;
  result->final_state = TCPIP_L07_TCP_STATE_CLOSED;

  if (scenario == NULL ||
      !tcpip_linux_l03_span_is_valid(
          scenario->payload, scenario->payload_length, sizeof(*scenario->payload)) ||
      !tcpip_linux_l03_span_is_valid(
          scenario->output, scenario->output_capacity, sizeof(*scenario->output)) ||
      !tcpip_linux_l03_span_is_valid(
          scenario->routes, scenario->route_count, sizeof(*scenario->routes)) ||
      !tcpip_linux_l03_span_is_valid(
          scenario->faults, scenario->fault_count, sizeof(*scenario->faults))) {
    return TCPIP_LINUX_L03_INVALID_ARGUMENT;
  }
  if (tcpip_linux_l03_spans_overlap(
          scenario->payload,
          scenario->payload_length,
          sizeof(*scenario->payload),
          result,
          1U,
          sizeof(*result)) ||
      tcpip_linux_l03_spans_overlap(
          scenario->output,
          scenario->output_capacity,
          sizeof(*scenario->output),
          result,
          1U,
          sizeof(*result)) ||
      tcpip_linux_l03_spans_overlap(
          scenario->routes,
          scenario->route_count,
          sizeof(*scenario->routes),
          result,
          1U,
          sizeof(*result)) ||
      tcpip_linux_l03_spans_overlap(
          scenario->faults,
          scenario->fault_count,
          sizeof(*scenario->faults),
          result,
          1U,
          sizeof(*result))) {
    return TCPIP_LINUX_L03_INVALID_ARGUMENT;
  }
  if (scenario->payload_length == 0U ||
      scenario->payload_length > TCPIP_LINUX_L03_MAX_PAYLOAD ||
      scenario->fault_count > TCPIP_LINUX_L03_MAX_FAULT_ACTIONS ||
      scenario->max_attempts == 0U || scenario->initial_rto_ms == 0U ||
      scenario->source_port == 0U || scenario->destination_port == 0U) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  for (size_t index = 0U; index < scenario->fault_count; index += 1U) {
    if (scenario->faults[index].action < TCPIP_LINUX_L03_DELIVER ||
        scenario->faults[index].action > TCPIP_LINUX_L03_REORDER) {
      return TCPIP_LINUX_L03_MALFORMED;
    }
  }
  if (scenario->output_capacity < scenario->payload_length) {
    return TCPIP_LINUX_L03_CAPACITY;
  }

  return TCPIP_LINUX_L03_TODO;
}
