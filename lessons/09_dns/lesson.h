#ifndef TCPIP_L09_DNS_LESSON_H
#define TCPIP_L09_DNS_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_l09_status {
  TCPIP_L09_OK = 0,
  TCPIP_L09_INVALID_ARGUMENT,
  TCPIP_L09_TRUNCATED,
  TCPIP_L09_MALFORMED,
  TCPIP_L09_CAPACITY,
  TCPIP_L09_TODO
} tcpip_l09_status;

typedef struct tcpip_l09_dns_header {
  uint16_t id;
  uint16_t flags;
  uint16_t question_count;
  uint16_t answer_count;
  uint16_t authority_count;
  uint16_t additional_count;
  uint8_t rcode;
} tcpip_l09_dns_header;

/*
 * Convert a dotted name to DNS wire form. name is not NUL-terminated unless
 * name_len includes such a byte (which is rejected). A trailing dot is
 * accepted; (NULL, 0) and (".", 1) both represent the root name. On success,
 * written is the encoded size including the root byte.
 */
tcpip_l09_status tcpip_l09_encode_name(
    const char *name,
    size_t name_len,
    uint8_t *out,
    size_t out_capacity,
    size_t *written);

/*
 * Decode one possibly compressed wire name. out receives dotted bytes and a
 * trailing NUL; written excludes that NUL. next_offset is the first byte after
 * the original encoded name, not after any followed pointer target. Each
 * non-NULL metadata output is cleared even when its required companion is NULL.
 */
tcpip_l09_status tcpip_l09_decode_name(
    const uint8_t *message,
    size_t message_len,
    size_t offset,
    uint8_t *out,
    size_t out_capacity,
    size_t *next_offset,
    size_t *written);

/* Build a standard recursive IN query containing one question. */
tcpip_l09_status tcpip_l09_build_query(
    uint16_t id,
    const char *name,
    size_t name_len,
    uint16_t qtype,
    uint8_t *out,
    size_t out_capacity,
    size_t *written);

/* Decode the fixed header to host-order values; no section is traversed. */
tcpip_l09_status tcpip_l09_parse_message(
    const uint8_t *message,
    size_t message_len,
    tcpip_l09_dns_header *header);

/*
 * Find the first IN A record in a successful, untruncated standard response.
 * The QR bit must identify a response, OPCODE and the reserved Z bit must be
 * zero, and RCODE must report success. Modern AD/CD flag bits are accepted.
 * An IN A record with RDLENGTH other than four is malformed. Each non-NULL
 * output is cleared even when its required companion is NULL.
 */
tcpip_l09_status tcpip_l09_first_a(
    const uint8_t *message,
    size_t message_len,
    uint8_t out[4],
    uint32_t *ttl);

#ifdef __cplusplus
}
#endif

#endif
