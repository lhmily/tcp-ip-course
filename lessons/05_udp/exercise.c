#include "lesson.h"

#include <string.h>

static int tcpip_l05_valid_ipv4_span(const uint8_t *address, size_t length) {
  return address != NULL && length == 4U;
}

tcpip_l05_status tcpip_l05_parse_datagram(
    const uint8_t *datagram,
    size_t datagram_length,
    tcpip_l05_datagram *out_datagram) {
  (void)datagram_length;

  if (out_datagram == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  memset(out_datagram, 0, sizeof(*out_datagram));
  if (datagram == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }

  /* TODO(lesson 05): decode the UDP header and validate its length field. */
  return TCPIP_L05_TODO;
}

tcpip_l05_status tcpip_l05_build_datagram(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    uint16_t source_port,
    uint16_t destination_port,
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_datagram_length) {
  (void)source_port;
  (void)destination_port;
  (void)destination_capacity;

  if (out_datagram_length == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  *out_datagram_length = 0U;
  if (!tcpip_l05_valid_ipv4_span(source_ipv4, source_ipv4_length) ||
      !tcpip_l05_valid_ipv4_span(destination_ipv4, destination_ipv4_length) ||
      (payload == NULL && payload_length != 0U) || destination == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }

  /* TODO(lesson 05): encode, then checksum, a capacity-checked UDP datagram. */
  return TCPIP_L05_TODO;
}

tcpip_l05_status tcpip_l05_ipv4_checksum(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    const uint8_t *datagram,
    size_t datagram_length,
    uint16_t *out_checksum) {
  (void)datagram_length;

  if (out_checksum == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  *out_checksum = 0U;
  if (!tcpip_l05_valid_ipv4_span(source_ipv4, source_ipv4_length) ||
      !tcpip_l05_valid_ipv4_span(destination_ipv4, destination_ipv4_length) ||
      datagram == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }

  /* TODO(lesson 05): sum the IPv4 pseudo-header and UDP bytes safely. */
  return TCPIP_L05_TODO;
}
