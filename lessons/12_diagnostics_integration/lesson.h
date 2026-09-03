#ifndef TCPIP_L12_LESSON_H
#define TCPIP_L12_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_L12_MAX_LAYERS 4U

typedef enum tcpip_l12_status {
  TCPIP_L12_OK = 0,
  TCPIP_L12_INVALID_ARGUMENT,
  TCPIP_L12_TRUNCATED,
  TCPIP_L12_MALFORMED,
  TCPIP_L12_CAPACITY,
  TCPIP_L12_TODO
} tcpip_l12_status;

typedef enum tcpip_l12_layer {
  TCPIP_L12_LAYER_ETHERNET = 0,
  TCPIP_L12_LAYER_ARP,
  TCPIP_L12_LAYER_IPV4,
  TCPIP_L12_LAYER_ICMP,
  TCPIP_L12_LAYER_UDP,
  TCPIP_L12_LAYER_DNS,
  TCPIP_L12_LAYER_TCP,
  TCPIP_L12_LAYER_HTTP
} tcpip_l12_layer;

typedef enum tcpip_l12_diagnostic {
  TCPIP_L12_DIAG_NONE = 0,
  TCPIP_L12_DIAG_TRUNCATED = 1U << 0,
  TCPIP_L12_DIAG_UNSUPPORTED = 1U << 1,
  TCPIP_L12_DIAG_MALFORMED = 1U << 2,
  TCPIP_L12_DIAG_CHECKSUM_MISMATCH = 1U << 3,
  TCPIP_L12_DIAG_IPV4_CHECKSUM = 1U << 4,
  TCPIP_L12_DIAG_TRANSPORT_CHECKSUM = 1U << 5
} tcpip_l12_diagnostic;

typedef struct tcpip_l12_report {
  tcpip_l12_layer layers[TCPIP_L12_MAX_LAYERS];
  size_t layer_count;
  uint32_t diagnostics;

  size_t frame_length;
  size_t parsed_length;
  size_t payload_length;

  uint16_t ether_type;
  uint16_t arp_opcode;
  uint8_t ip_protocol;
  uint32_t source_ipv4;
  uint32_t destination_ipv4;
  uint16_t source_port;
  uint16_t destination_port;

  uint8_t icmp_type;
  uint16_t dns_identifier;
  uint16_t dns_question_count;
  uint8_t tcp_flags;
  size_t http_message_length;

  unsigned checksums_checked;
  unsigned checksums_valid;
} tcpip_l12_report;

/*
 * Diagnoses one complete Ethernet frame. Unsupported protocols are reported
 * through diagnostics and still return TCPIP_L12_OK. Structural errors return
 * TCPIP_L12_TRUNCATED or TCPIP_L12_MALFORMED after preserving partial results.
 */
tcpip_l12_status tcpip_l12_diagnose_frame(
    const uint8_t *data, size_t len, tcpip_l12_report *report);

/*
 * Writes a deterministic one-line summary. After written itself is validated,
 * *written is set to zero before any other validation; success or a short
 * output buffer then replaces it with the required byte count excluding the
 * terminating NUL. A short output buffer receives a NUL-terminated prefix when
 * cap is nonzero and returns TCPIP_L12_CAPACITY.
 */
tcpip_l12_status tcpip_l12_format_report(
    const tcpip_l12_report *report, char *out, size_t cap, size_t *written);

#ifdef __cplusplus
}
#endif

#endif
