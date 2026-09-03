#include "lesson.h"

#include <string.h>

tcpip_l03_status tcpip_l03_parse_ipv4(
    const uint8_t *packet,
    size_t packet_length,
    tcpip_l03_ipv4_packet *out_packet) {
  if (out_packet != NULL) {
    memset(out_packet, 0, sizeof(*out_packet));
  }
  if (packet == NULL || out_packet == NULL) {
    return TCPIP_L03_INVALID_ARGUMENT;
  }
  if (packet_length < TCPIP_L03_MIN_HEADER_LENGTH) {
    return TCPIP_L03_TRUNCATED;
  }
  if (((((uint16_t)packet[6U] << 8U) | (uint16_t)packet[7U]) &
       UINT16_C(0x8000)) != 0U) {
    return TCPIP_L03_MALFORMED;
  }

  /* TODO(lesson 03): validate version, IHL, lengths, options, and checksum before publishing fields. */
  return TCPIP_L03_TODO;
}

tcpip_l03_status tcpip_l03_build_header(
    const tcpip_l03_ipv4_header_fields *fields,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_header_length) {
  size_t header_length;

  if (out_header_length != NULL) {
    *out_header_length = 0U;
  }
  if (fields == NULL || destination == NULL || out_header_length == NULL) {
    return TCPIP_L03_INVALID_ARGUMENT;
  }
  if ((fields->options == NULL && fields->options_length != 0U) ||
      fields->options_length > TCPIP_L03_MAX_OPTIONS_LENGTH ||
      (fields->options_length % 4U) != 0U ||
      (fields->flags_fragment & UINT16_C(0x8000)) != 0U) {
    return TCPIP_L03_MALFORMED;
  }

  header_length = TCPIP_L03_MIN_HEADER_LENGTH + fields->options_length;
  if (destination_capacity < header_length) {
    return TCPIP_L03_CAPACITY;
  }

  /* TODO(lesson 03): assemble the complete header locally and insert its one's-complement checksum. */
  return TCPIP_L03_TODO;
}

tcpip_l03_status tcpip_l03_decrement_ttl(
    uint8_t *packet,
    size_t packet_length) {
  if (packet == NULL) {
    return TCPIP_L03_INVALID_ARGUMENT;
  }
  if (packet_length < TCPIP_L03_MIN_HEADER_LENGTH) {
    return TCPIP_L03_TRUNCATED;
  }

  /* TODO(lesson 03): validate the packet, reject TTL zero or one, then update TTL and checksum atomically. */
  return TCPIP_L03_TODO;
}
