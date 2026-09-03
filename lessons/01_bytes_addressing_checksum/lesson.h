#ifndef TCPIP_L01_LESSON_H
#define TCPIP_L01_LESSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_l01_status {
  TCPIP_L01_OK = 0,
  TCPIP_L01_INVALID_ARGUMENT,
  TCPIP_L01_TRUNCATED,
  TCPIP_L01_MALFORMED,
  TCPIP_L01_CAPACITY,
  TCPIP_L01_TODO
} tcpip_l01_status;

tcpip_l01_status tcpip_l01_read_be16(
    const uint8_t *data, size_t data_len, size_t offset, uint16_t *out_value);

tcpip_l01_status tcpip_l01_write_be16(
    uint8_t *dst, size_t dst_len, size_t offset, uint16_t value);

tcpip_l01_status tcpip_l01_parse_ipv4(
    const char *text,
    size_t text_len,
    uint8_t *out_address,
    size_t out_address_capacity);

tcpip_l01_status tcpip_l01_prefix_contains(
    const uint8_t *address,
    size_t address_length,
    const uint8_t *network,
    size_t network_length,
    uint8_t prefix_length,
    bool *out_contains);

tcpip_l01_status tcpip_l01_checksum16(
    const uint8_t *data, size_t data_len, uint16_t *out_checksum);

#ifdef __cplusplus
}
#endif

#endif
