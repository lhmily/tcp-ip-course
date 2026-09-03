#ifndef TCPIP_L11_ROUTING_NAT_LESSON_H
#define TCPIP_L11_ROUTING_NAT_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_l11_status {
  TCPIP_L11_OK = 0,
  TCPIP_L11_INVALID_ARGUMENT = 1,
  TCPIP_L11_TRUNCATED = 2,
  TCPIP_L11_MALFORMED = 3,
  TCPIP_L11_CAPACITY = 4,
  TCPIP_L11_TODO = 5
} tcpip_l11_status;

typedef struct tcpip_l11_route {
  uint8_t network[4];
  uint8_t prefix_length;
  uint8_t next_hop[4];
  uint32_t interface_index;
  uint32_t metric;
} tcpip_l11_route;

typedef struct tcpip_l11_tuple {
  uint8_t protocol;
  uint8_t source_ip[4];
  uint8_t destination_ip[4];
  uint16_t source_port;
  uint16_t destination_port;
} tcpip_l11_tuple;

typedef struct tcpip_l11_nat_mapping {
  uint8_t active;
  uint8_t protocol;
  uint8_t private_ip[4];
  uint8_t remote_ip[4];
  uint16_t private_port;
  uint16_t remote_port;
  uint16_t public_port;
  uint64_t last_used;
} tcpip_l11_nat_mapping;

typedef struct tcpip_l11_nat {
  uint8_t public_ip[4];
  uint16_t first_port;
  tcpip_l11_nat_mapping *storage;
  size_t capacity;
} tcpip_l11_nat;

/*
 * Selects the longest matching canonical route. Equal prefix lengths prefer
 * lower metrics, then earlier array order. *index is SIZE_MAX on failure.
 * TCPIP_L11_TRUNCATED means that no route matched the destination.
 */
tcpip_l11_status tcpip_l11_longest_prefix(
    const tcpip_l11_route *routes,
    size_t count,
    const uint8_t destination_ip[4],
    size_t *index);

/* Initializes a NAT over caller-owned mapping storage; first_port must be nonzero. */
tcpip_l11_status tcpip_l11_nat_init(
    tcpip_l11_nat *nat,
    uint16_t first_port,
    const uint8_t public_ip[4],
    tcpip_l11_nat_mapping *storage,
    size_t capacity);

/* Reuses or creates an endpoint-dependent outbound mapping. */
tcpip_l11_status tcpip_l11_nat_translate_outbound(
    tcpip_l11_nat *nat,
    uint64_t now,
    const tcpip_l11_tuple *tuple,
    tcpip_l11_tuple *out);

/* Performs reverse translation only; it never creates a mapping. */
tcpip_l11_status tcpip_l11_nat_translate_inbound(
    tcpip_l11_nat *nat,
    uint64_t now,
    const tcpip_l11_tuple *tuple,
    tcpip_l11_tuple *out);

/* Expires mappings idle for at least idle time units and reports their count. */
tcpip_l11_status tcpip_l11_nat_expire(
    tcpip_l11_nat *nat,
    uint64_t now,
    uint64_t idle,
    size_t *count);

#ifdef __cplusplus
}
#endif

#endif
