#ifndef TCPIP_L06_LESSON_H
#define TCPIP_L06_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_l06_status {
  TCPIP_L06_OK = 0,
  TCPIP_L06_INVALID_ARGUMENT,
  TCPIP_L06_TRUNCATED,
  TCPIP_L06_MALFORMED,
  TCPIP_L06_CAPACITY,
  TCPIP_L06_TODO
} tcpip_l06_status;

enum {
  TCPIP_L06_FLAG_FIN = 0x001,
  TCPIP_L06_FLAG_SYN = 0x002,
  TCPIP_L06_FLAG_RST = 0x004,
  TCPIP_L06_FLAG_PSH = 0x008,
  TCPIP_L06_FLAG_ACK = 0x010,
  TCPIP_L06_FLAG_URG = 0x020,
  TCPIP_L06_FLAG_ECE = 0x040,
  TCPIP_L06_FLAG_CWR = 0x080,
  TCPIP_L06_FLAG_NS = 0x100
};

typedef struct tcpip_l06_segment {
  uint16_t source_port;
  uint16_t destination_port;
  uint32_t sequence_number;
  uint32_t acknowledgment_number;
  uint8_t data_offset;
  uint16_t flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent_pointer;
  size_t options_offset;
  size_t options_length;
  size_t payload_offset;
  size_t payload_length;
} tcpip_l06_segment;

typedef struct tcpip_l06_segment_fields {
  const uint8_t *source_ipv4;
  size_t source_ipv4_length;
  const uint8_t *destination_ipv4;
  size_t destination_ipv4_length;
  uint16_t source_port;
  uint16_t destination_port;
  uint32_t sequence_number;
  uint32_t acknowledgment_number;
  uint16_t flags;
  uint16_t window;
  uint16_t urgent_pointer;
  const uint8_t *options;
  size_t options_length;
} tcpip_l06_segment_fields;

/* Parse structural TCP fields; checksum validation is deliberately separate. */
tcpip_l06_status tcpip_l06_parse_segment(
    const uint8_t *segment,
    size_t segment_length,
    tcpip_l06_segment *out_segment);

/*
 * Build and checksum one TCP segment. Options are supplied in fields and must
 * be 0..40 bytes and a multiple of four. IPv4 spans must each be four bytes.
 * The options and payload input spans must not overlap the destination span.
 */
tcpip_l06_status tcpip_l06_build_segment(
    const tcpip_l06_segment_fields *fields,
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_segment_length);

/* A complete valid checksummed TCP segment produces a result of zero. */
tcpip_l06_status tcpip_l06_ipv4_checksum(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    const uint8_t *segment,
    size_t segment_length,
    uint16_t *out_checksum);

/* RFC-style serial comparison; exactly half the sequence space is unordered. */
int tcpip_l06_seq_before(uint32_t first, uint32_t second);

/* Return the modular forward distance second - first. */
uint32_t tcpip_l06_seq_distance(uint32_t first, uint32_t second);

#ifdef __cplusplus
}
#endif

#endif
