#ifndef TCPIP_L03_LESSON_H
#define TCPIP_L03_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_L03_IPV4_ADDRESS_LENGTH 4U
#define TCPIP_L03_MIN_HEADER_LENGTH 20U
#define TCPIP_L03_MAX_HEADER_LENGTH 60U
#define TCPIP_L03_MAX_OPTIONS_LENGTH 40U

typedef enum tcpip_l03_status {
  TCPIP_L03_OK = 0,
  TCPIP_L03_INVALID_ARGUMENT,
  TCPIP_L03_TRUNCATED,
  TCPIP_L03_MALFORMED,
  TCPIP_L03_CAPACITY,
  TCPIP_L03_TODO
} tcpip_l03_status;

typedef struct tcpip_l03_ipv4_packet {
  uint8_t version;
  uint8_t ihl;
  uint8_t dscp_ecn;
  uint16_t total_length;
  uint16_t identification;
  uint16_t flags_fragment;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t header_checksum;
  uint8_t source[TCPIP_L03_IPV4_ADDRESS_LENGTH];
  uint8_t destination[TCPIP_L03_IPV4_ADDRESS_LENGTH];
  size_t options_offset;
  size_t options_length;
  size_t payload_offset;
  size_t payload_length;
} tcpip_l03_ipv4_packet;

typedef struct tcpip_l03_ipv4_header_fields {
  uint8_t dscp_ecn;
  uint16_t identification;
  uint16_t flags_fragment;
  uint8_t ttl;
  uint8_t protocol;
  uint8_t source[TCPIP_L03_IPV4_ADDRESS_LENGTH];
  uint8_t destination[TCPIP_L03_IPV4_ADDRESS_LENGTH];
  const uint8_t *options;
  size_t options_length;
} tcpip_l03_ipv4_header_fields;

/* Parse one IPv4 packet from a possibly larger containing byte span. */
tcpip_l03_status tcpip_l03_parse_ipv4(
    const uint8_t *packet,
    size_t packet_length,
    tcpip_l03_ipv4_packet *out_packet);

/* Build a checksummed IPv4 header whose total length is the header length. */
tcpip_l03_status tcpip_l03_build_header(
    const tcpip_l03_ipv4_header_fields *fields,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_header_length);

/* Decrement TTL in a valid IPv4 packet and repair its header checksum. */
tcpip_l03_status tcpip_l03_decrement_ttl(
    uint8_t *packet,
    size_t packet_length);

#ifdef __cplusplus
}
#endif

#endif
