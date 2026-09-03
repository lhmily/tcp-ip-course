#include "lab.h"

#include <string.h>

static int tcpip_linux_l03_span_is_valid(const void *span, size_t length) {
  return length == 0U || span != NULL;
}

tcpip_linux_l03_status tcpip_linux_l03_run(
    const tcpip_linux_l03_scenario *scenario,
    tcpip_linux_l03_result *result) {
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
  if (scenario->payload_length > TCPIP_LINUX_L03_MAX_PAYLOAD ||
      scenario->fault_count > TCPIP_LINUX_L03_MAX_FAULT_ACTIONS ||
      scenario->max_attempts == 0U || scenario->initial_rto_ms == 0U ||
      scenario->source_port == 0U || scenario->destination_port == 0U) {
    return TCPIP_LINUX_L03_MALFORMED;
  }
  if (scenario->output_capacity < scenario->payload_length) {
    return TCPIP_LINUX_L03_CAPACITY;
  }

  return TCPIP_LINUX_L03_TODO;
}
