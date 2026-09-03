#ifndef TCPIP_L04_LESSON_H
#define TCPIP_L04_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_L04_HEADER_LENGTH 4U
#define TCPIP_L04_ECHO_HEADER_LENGTH 8U
#define TCPIP_L04_ECHO_REPLY 0U
#define TCPIP_L04_ECHO_REQUEST 8U

typedef enum tcpip_l04_status {
  TCPIP_L04_OK = 0,
  TCPIP_L04_INVALID_ARGUMENT,
  TCPIP_L04_TRUNCATED,
  TCPIP_L04_MALFORMED,
  TCPIP_L04_CAPACITY,
  TCPIP_L04_TODO
} tcpip_l04_status;

typedef struct tcpip_l04_message {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  size_t body_offset;
  size_t body_length;
} tcpip_l04_message;

/* Parse any complete ICMP message and verify its one's-complement checksum. */
tcpip_l04_status tcpip_l04_parse_message(
    const uint8_t *message,
    size_t message_length,
    tcpip_l04_message *out_message);

/* Build an echo request (type 8) or echo reply (type 0), always with code zero. */
tcpip_l04_status tcpip_l04_build_echo(
    uint8_t type,
    uint16_t identifier,
    uint16_t sequence,
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_message_length);

/* Validate an echo request and build a reply preserving identifier, sequence, and payload. */
tcpip_l04_status tcpip_l04_make_echo_reply(
    const uint8_t *request,
    size_t request_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_reply_length);

#ifdef __cplusplus
}
#endif

#endif
