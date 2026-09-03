#include "lab.h"

#include <stdio.h>

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
  return TCPIP_LINUX_L04_TODO;
}

tcpip_linux_l04_status tcpip_linux_l04_validate_path(
    const tcpip_linux_l04_symbol_id *path,
    size_t path_length) {
  size_t index;
  if ((path == NULL && path_length != 0U) || path_length == 0U) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  for (index = 0U; index < path_length; index += 1U) {
    if (!tcpip_linux_l04_id_in_range(path[index])) {
      return TCPIP_LINUX_L04_NOT_FOUND;
    }
  }
  return TCPIP_LINUX_L04_TODO;
}

tcpip_linux_l04_status tcpip_linux_l04_format_source_url(
    tcpip_linux_l04_symbol_id id,
    char *output,
    size_t output_capacity,
    size_t *written) {
  if (written == NULL) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  *written = 0U;
  if ((output == NULL && output_capacity != 0U) || !tcpip_linux_l04_id_in_range(id)) {
    return output == NULL && output_capacity != 0U
               ? TCPIP_LINUX_L04_INVALID_ARGUMENT
               : TCPIP_LINUX_L04_NOT_FOUND;
  }
  if (output_capacity != 0U) {
    output[0] = '\0';
  }
  return TCPIP_LINUX_L04_TODO;
}

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_ingress_path(size_t *count) {
  if (count != NULL) {
    *count = 0U;
  }
  return NULL;
}

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_egress_path(size_t *count) {
  if (count != NULL) {
    *count = 0U;
  }
  return NULL;
}
