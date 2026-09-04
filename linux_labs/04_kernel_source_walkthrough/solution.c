#include "lab.h"

#include <stdio.h>

/* walkthrough_data.inc is generated from the visual walkthrough's canonical data. */
typedef struct tcpip_linux_l04_edge {
  tcpip_linux_l04_symbol_id from;
  tcpip_linux_l04_symbol_id to;
} tcpip_linux_l04_edge;

#include "walkthrough_data.inc"

#define TCPIP_LINUX_L04_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

_Static_assert(
    TCPIP_LINUX_L04_GENERATED_SYMBOL_COUNT == TCPIP_LINUX_L04_SYMBOL_COUNT,
    "generated symbol count must match the public symbol count");
_Static_assert(
    TCPIP_LINUX_L04_GENERATED_INGRESS_COUNT == TCPIP_LINUX_L04_INGRESS_COUNT,
    "generated ingress count must match the public ingress count");
_Static_assert(
    TCPIP_LINUX_L04_GENERATED_EGRESS_COUNT == TCPIP_LINUX_L04_EGRESS_COUNT,
    "generated egress count must match the public egress count");
_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_symbols) ==
        TCPIP_LINUX_L04_GENERATED_SYMBOL_COUNT,
    "generated symbol array count is inconsistent");
_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_ingress) ==
        TCPIP_LINUX_L04_GENERATED_INGRESS_COUNT,
    "generated ingress array count is inconsistent");
_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_egress) ==
        TCPIP_LINUX_L04_GENERATED_EGRESS_COUNT,
    "generated egress array count is inconsistent");
_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_edges) ==
        TCPIP_LINUX_L04_GENERATED_EDGE_COUNT,
    "generated edge array count is inconsistent");
_Static_assert(TCPIP_LINUX_L04_GENERATED_EDGE_COUNT > 0U, "generated edge array must not be empty");

static int tcpip_linux_l04_id_in_range(tcpip_linux_l04_symbol_id id) {
  return id >= TCPIP_LINUX_L04_NETIF_RECEIVE_SKB && id <= TCPIP_LINUX_L04_TCP_GET_INFO;
}

tcpip_linux_l04_status tcpip_linux_l04_symbol_by_id(
    tcpip_linux_l04_symbol_id id,
    const tcpip_linux_l04_symbol **out_symbol) {
  if (out_symbol == NULL) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  *out_symbol = NULL;
  if (!tcpip_linux_l04_id_in_range(id)) {
    return TCPIP_LINUX_L04_NOT_FOUND;
  }
  *out_symbol = &tcpip_linux_l04_symbols[(size_t)id];
  return TCPIP_LINUX_L04_OK;
}

tcpip_linux_l04_status tcpip_linux_l04_validate_path(
    const tcpip_linux_l04_symbol_id *path,
    size_t path_length) {
  size_t index;
  size_t edge_index;

  if (path == NULL || path_length == 0U) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  for (index = 0U; index < path_length; index += 1U) {
    if (!tcpip_linux_l04_id_in_range(path[index])) {
      return TCPIP_LINUX_L04_NOT_FOUND;
    }
  }
  for (index = 1U; index < path_length; index += 1U) {
    int found = 0;
    for (edge_index = 0U; edge_index < TCPIP_LINUX_L04_GENERATED_EDGE_COUNT; edge_index += 1U) {
      if (tcpip_linux_l04_edges[edge_index].from == path[index - 1U] &&
          tcpip_linux_l04_edges[edge_index].to == path[index]) {
        found = 1;
        break;
      }
    }
    if (found == 0) {
      return TCPIP_LINUX_L04_INVALID_PATH;
    }
  }
  return TCPIP_LINUX_L04_OK;
}

tcpip_linux_l04_status tcpip_linux_l04_format_source_url(
    tcpip_linux_l04_symbol_id id,
    char *output,
    size_t output_capacity,
    size_t *written) {
  const tcpip_linux_l04_symbol *symbol = NULL;
  int required;

  if (written != NULL) {
    *written = 0U;
  }
  if (output != NULL && output_capacity > 0U) {
    output[0] = '\0';
  }
  if (written == NULL) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  if (output == NULL && output_capacity != 0U) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  if (tcpip_linux_l04_symbol_by_id(id, &symbol) != TCPIP_LINUX_L04_OK) {
    return TCPIP_LINUX_L04_NOT_FOUND;
  }
  required = snprintf(
      NULL,
      0U,
      "https://github.com/torvalds/linux/blob/v6.6/%s#L%u",
      symbol->source_path,
      symbol->source_line);
  if (required < 0) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  *written = (size_t)required;
  if (output_capacity == 0U || (size_t)required >= output_capacity) {
    return TCPIP_LINUX_L04_CAPACITY;
  }
  required = snprintf(
      output,
      output_capacity,
      "https://github.com/torvalds/linux/blob/v6.6/%s#L%u",
      symbol->source_path,
      symbol->source_line);
  if (required < 0) {
    output[0] = '\0';
    *written = 0U;
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  return TCPIP_LINUX_L04_OK;
}

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_ingress_path(size_t *count) {
  if (count == NULL) {
    return NULL;
  }
  *count = TCPIP_LINUX_L04_GENERATED_INGRESS_COUNT;
  return tcpip_linux_l04_ingress;
}

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_egress_path(size_t *count) {
  if (count == NULL) {
    return NULL;
  }
  *count = TCPIP_LINUX_L04_GENERATED_EGRESS_COUNT;
  return tcpip_linux_l04_egress;
}
