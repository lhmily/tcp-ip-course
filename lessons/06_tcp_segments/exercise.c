#include "lesson.h"

#include <string.h>

static int tcpip_l06_valid_ipv4_span(const uint8_t *address, size_t length) {
  return address != NULL && length == 4U;
}

tcpip_l06_status tcpip_l06_parse_segment(
    const uint8_t *segment,
    size_t segment_length,
    tcpip_l06_segment *out_segment) {
  (void)segment_length;

  if (out_segment == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  memset(out_segment, 0, sizeof(*out_segment));
  if (segment == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }

  /* TODO(lesson 06): decode the fixed header and validate the data offset. */
  return TCPIP_L06_TODO;
}

tcpip_l06_status tcpip_l06_build_segment(
    const tcpip_l06_segment_fields *fields,
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_segment_length) {
  (void)destination_capacity;

  if (out_segment_length == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  *out_segment_length = 0U;
  if (fields == NULL || destination == NULL ||
      (payload == NULL && payload_length != 0U)) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  if (!tcpip_l06_valid_ipv4_span(fields->source_ipv4, fields->source_ipv4_length) ||
      !tcpip_l06_valid_ipv4_span(
          fields->destination_ipv4, fields->destination_ipv4_length) ||
      (fields->options == NULL && fields->options_length != 0U)) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }

  /* TODO(lesson 06): validate options, encode fields, and generate checksum. */
  return TCPIP_L06_TODO;
}

tcpip_l06_status tcpip_l06_ipv4_checksum(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    const uint8_t *segment,
    size_t segment_length,
    uint16_t *out_checksum) {
  (void)segment_length;

  if (out_checksum == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  *out_checksum = 0U;
  if (!tcpip_l06_valid_ipv4_span(source_ipv4, source_ipv4_length) ||
      !tcpip_l06_valid_ipv4_span(destination_ipv4, destination_ipv4_length) ||
      segment == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }

  /* TODO(lesson 06): checksum the IPv4 pseudo-header and TCP bytes. */
  return TCPIP_L06_TODO;
}

int tcpip_l06_seq_before(uint32_t first, uint32_t second) {
  (void)first;
  (void)second;
  /* TODO(lesson 06): compare serial numbers across wrap without signed casts. */
  return 0;
}

uint32_t tcpip_l06_seq_distance(uint32_t first, uint32_t second) {
  (void)first;
  (void)second;
  /* TODO(lesson 06): compute the modular forward distance. */
  return 0U;
}
