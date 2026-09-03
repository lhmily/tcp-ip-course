#include "lesson.h"

#include <string.h>

tcpip_l11_status tcpip_l11_longest_prefix(
    const tcpip_l11_route *routes,
    size_t count,
    const uint8_t destination_ip[4],
    size_t *index) {
  if (index == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  *index = SIZE_MAX;
  if ((routes == NULL && count != 0u) || destination_ip == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  return TCPIP_L11_TODO;
}

tcpip_l11_status tcpip_l11_nat_init(
    tcpip_l11_nat *nat,
    uint16_t first_port,
    const uint8_t public_ip[4],
    tcpip_l11_nat_mapping *storage,
    size_t capacity) {
  if (nat == NULL || first_port == 0u || public_ip == NULL || storage == NULL ||
      capacity == 0u) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  return TCPIP_L11_TODO;
}

tcpip_l11_status tcpip_l11_nat_translate_outbound(
    tcpip_l11_nat *nat,
    uint64_t now,
    const tcpip_l11_tuple *tuple,
    tcpip_l11_tuple *out) {
  (void)now;
  if (out == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  memset(out, 0, sizeof(*out));
  if (nat == NULL || tuple == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  return TCPIP_L11_TODO;
}

tcpip_l11_status tcpip_l11_nat_translate_inbound(
    tcpip_l11_nat *nat,
    uint64_t now,
    const tcpip_l11_tuple *tuple,
    tcpip_l11_tuple *out) {
  (void)now;
  if (out == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  memset(out, 0, sizeof(*out));
  if (nat == NULL || tuple == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  return TCPIP_L11_TODO;
}

tcpip_l11_status tcpip_l11_nat_expire(
    tcpip_l11_nat *nat, uint64_t now, uint64_t idle, size_t *count) {
  (void)now;
  (void)idle;
  if (count == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  *count = 0u;
  if (nat == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  return TCPIP_L11_TODO;
}
