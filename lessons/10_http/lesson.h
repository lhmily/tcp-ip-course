#ifndef TCPIP_L10_HTTP_LESSON_H
#define TCPIP_L10_HTTP_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_L10_MAX_HEADERS 16u
#define TCPIP_L10_MAX_HEADER_BYTES 4096u
#define TCPIP_L10_MAX_BODY_BYTES 8192u

typedef enum tcpip_l10_status {
  TCPIP_L10_OK = 0,
  TCPIP_L10_INVALID_ARGUMENT = 1,
  TCPIP_L10_TRUNCATED = 2,
  TCPIP_L10_MALFORMED = 3,
  TCPIP_L10_CAPACITY = 4,
  TCPIP_L10_TODO = 5,
  TCPIP_L10_EOF = 6,
  TCPIP_L10_TIMEOUT = 7,
  TCPIP_L10_SYSTEM = 8
} tcpip_l10_status;

typedef struct tcpip_l10_span {
  const uint8_t *data;
  size_t length;
} tcpip_l10_span;

typedef struct tcpip_l10_header_view {
  tcpip_l10_span name;
  tcpip_l10_span value;
} tcpip_l10_header_view;

typedef enum tcpip_l10_message_kind {
  TCPIP_L10_MESSAGE_REQUEST = 1,
  TCPIP_L10_MESSAGE_RESPONSE = 2
} tcpip_l10_message_kind;

typedef struct tcpip_l10_message_view {
  tcpip_l10_message_kind kind;
  tcpip_l10_span method;
  tcpip_l10_span target;
  tcpip_l10_span version;
  unsigned status_code;
  tcpip_l10_span reason;
  tcpip_l10_header_view headers[TCPIP_L10_MAX_HEADERS];
  size_t header_count;
  tcpip_l10_span body;
  size_t content_length;
  size_t message_length;
} tcpip_l10_message_view;

/* Builds: POST <path> HTTP/1.1, Host: localhost, Content-Length, Connection: close. */
tcpip_l10_status tcpip_l10_build_request(
    const uint8_t *path,
    size_t path_len,
    const uint8_t *body,
    size_t body_len,
    uint8_t *out,
    size_t cap,
    size_t *written);

/* Parses exactly one complete message from the caller-owned byte span. */
tcpip_l10_status tcpip_l10_parse_message(
    const uint8_t *input,
    size_t input_len,
    tcpip_l10_message_view *message);

/* Sends all bytes before timeout_ms elapses. timeout_ms must be nonnegative. */
tcpip_l10_status tcpip_l10_send_message(
    int fd,
    const uint8_t *message,
    size_t message_len,
    int timeout_ms,
    size_t *sent);

/* Receives one Content-Length-framed message into out and returns its parsed view. */
tcpip_l10_status tcpip_l10_recv_message(
    int fd,
    uint8_t *out,
    size_t cap,
    int timeout_ms,
    size_t *received,
    tcpip_l10_message_view *message);

/* Receives one request and sends a fixed response under one absolute deadline. */
tcpip_l10_status tcpip_l10_serve_one(
    int fd,
    uint8_t *request_buffer,
    size_t request_capacity,
    int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
