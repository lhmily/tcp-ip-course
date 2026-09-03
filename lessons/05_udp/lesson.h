#ifndef TCPIP_L05_LESSON_H
#define TCPIP_L05_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_l05_status {
  TCPIP_L05_OK = 0,
  TCPIP_L05_INVALID_ARGUMENT,
  TCPIP_L05_TRUNCATED,
  TCPIP_L05_MALFORMED,
  TCPIP_L05_CAPACITY,
  TCPIP_L05_TODO
} tcpip_l05_status;

typedef struct tcpip_l05_datagram {
  uint16_t source_port;
  uint16_t destination_port;
  uint16_t length;
  uint16_t checksum;
  size_t payload_offset;
  size_t payload_length;
} tcpip_l05_datagram;

/*
 * Parse one complete UDP datagram. Under this lesson's strict framing policy,
 * datagram_length must equal the UDP length field. A zero checksum is accepted
 * because IPv4 permits senders to omit the UDP checksum.
 */
tcpip_l05_status tcpip_l05_parse_datagram(
    const uint8_t *datagram,
    size_t datagram_length,
    tcpip_l05_datagram *out_datagram);

/*
 * Build a UDP datagram and generate its IPv4 pseudo-header checksum.
 * source_ipv4_length and destination_ipv4_length must both be exactly 4.
 * A computed checksum of zero is encoded as 0xffff, per UDP over IPv4.
 */
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
    size_t *out_datagram_length);

/*
 * Compute the one's-complement result over the IPv4 UDP pseudo-header and the
 * supplied datagram bytes. A complete valid checksummed datagram yields zero.
 */
tcpip_l05_status tcpip_l05_ipv4_checksum(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    const uint8_t *datagram,
    size_t datagram_length,
    uint16_t *out_checksum);

#ifdef __cplusplus
}
#endif

#endif
