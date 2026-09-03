#include "lesson.h"

#include <limits.h>
#include <string.h>

#define TCPIP_L05_UDP_HEADER_LENGTH 8U
#define TCPIP_L05_UDP_PROTOCOL 17U

static uint16_t tcpip_l05_read_u16(const uint8_t *bytes) {
  return (uint16_t)(((uint16_t)bytes[0] << 8U) | (uint16_t)bytes[1]);
}

static void tcpip_l05_write_u16(uint8_t *bytes, uint16_t value) {
  bytes[0] = (uint8_t)(value >> 8U);
  bytes[1] = (uint8_t)value;
}

static int tcpip_l05_valid_ipv4_span(const uint8_t *address, size_t length) {
  return address != NULL && length == 4U;
}

static uint32_t tcpip_l05_add_word(uint32_t sum, uint16_t word) {
  sum += (uint32_t)word;
  return (sum & UINT32_C(0xffff)) + (sum >> 16U);
}

static uint32_t tcpip_l05_add_bytes(uint32_t sum, const uint8_t *bytes, size_t length) {
  size_t index = 0U;

  while (length - index >= 2U) {
    sum = tcpip_l05_add_word(sum, tcpip_l05_read_u16(bytes + index));
    index += 2U;
  }
  if (index < length) {
    sum = tcpip_l05_add_word(sum, (uint16_t)((uint16_t)bytes[index] << 8U));
  }
  return sum;
}

static uint16_t tcpip_l05_finish_checksum(uint32_t sum) {
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)~(uint16_t)sum;
}

static uint16_t tcpip_l05_checksum_unchecked(
    const uint8_t source_ipv4[4],
    const uint8_t destination_ipv4[4],
    const uint8_t *datagram,
    size_t datagram_length) {
  uint32_t sum = 0U;

  sum = tcpip_l05_add_bytes(sum, source_ipv4, 4U);
  sum = tcpip_l05_add_bytes(sum, destination_ipv4, 4U);
  sum = tcpip_l05_add_word(sum, (uint16_t)TCPIP_L05_UDP_PROTOCOL);
  sum = tcpip_l05_add_word(sum, (uint16_t)datagram_length);
  sum = tcpip_l05_add_bytes(sum, datagram, datagram_length);
  return tcpip_l05_finish_checksum(sum);
}

tcpip_l05_status tcpip_l05_parse_datagram(
    const uint8_t *datagram,
    size_t datagram_length,
    tcpip_l05_datagram *out_datagram) {
  uint16_t declared_length;

  if (out_datagram == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  memset(out_datagram, 0, sizeof(*out_datagram));
  if (datagram == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  if (datagram_length < TCPIP_L05_UDP_HEADER_LENGTH) {
    return TCPIP_L05_TRUNCATED;
  }

  declared_length = tcpip_l05_read_u16(datagram + 4U);
  if (declared_length < TCPIP_L05_UDP_HEADER_LENGTH) {
    return TCPIP_L05_MALFORMED;
  }
  if (datagram_length < (size_t)declared_length) {
    return TCPIP_L05_TRUNCATED;
  }
  if (datagram_length != (size_t)declared_length) {
    return TCPIP_L05_MALFORMED;
  }

  out_datagram->source_port = tcpip_l05_read_u16(datagram);
  out_datagram->destination_port = tcpip_l05_read_u16(datagram + 2U);
  out_datagram->length = declared_length;
  out_datagram->checksum = tcpip_l05_read_u16(datagram + 6U);
  out_datagram->payload_offset = TCPIP_L05_UDP_HEADER_LENGTH;
  out_datagram->payload_length = datagram_length - TCPIP_L05_UDP_HEADER_LENGTH;
  return TCPIP_L05_OK;
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
  uint8_t source_copy[4];
  uint8_t destination_copy[4];
  size_t datagram_length;
  uint16_t checksum;

  if (out_datagram_length == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  *out_datagram_length = 0U;
  if (!tcpip_l05_valid_ipv4_span(source_ipv4, source_ipv4_length) ||
      !tcpip_l05_valid_ipv4_span(destination_ipv4, destination_ipv4_length) ||
      (payload == NULL && payload_length != 0U) || destination == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  if (payload_length > (size_t)UINT16_MAX - TCPIP_L05_UDP_HEADER_LENGTH) {
    return TCPIP_L05_MALFORMED;
  }
  datagram_length = TCPIP_L05_UDP_HEADER_LENGTH + payload_length;
  if (destination_capacity < datagram_length) {
    return TCPIP_L05_CAPACITY;
  }

  memcpy(source_copy, source_ipv4, sizeof(source_copy));
  memcpy(destination_copy, destination_ipv4, sizeof(destination_copy));
  if (payload_length != 0U) {
    memmove(destination + TCPIP_L05_UDP_HEADER_LENGTH, payload, payload_length);
  }
  tcpip_l05_write_u16(destination, source_port);
  tcpip_l05_write_u16(destination + 2U, destination_port);
  tcpip_l05_write_u16(destination + 4U, (uint16_t)datagram_length);
  tcpip_l05_write_u16(destination + 6U, 0U);

  checksum = tcpip_l05_checksum_unchecked(
      source_copy, destination_copy, destination, datagram_length);
  if (checksum == 0U) {
    checksum = UINT16_MAX;
  }
  tcpip_l05_write_u16(destination + 6U, checksum);
  *out_datagram_length = datagram_length;
  return TCPIP_L05_OK;
}

tcpip_l05_status tcpip_l05_ipv4_checksum(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    const uint8_t *datagram,
    size_t datagram_length,
    uint16_t *out_checksum) {
  if (out_checksum == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  *out_checksum = 0U;
  if (!tcpip_l05_valid_ipv4_span(source_ipv4, source_ipv4_length) ||
      !tcpip_l05_valid_ipv4_span(destination_ipv4, destination_ipv4_length) ||
      datagram == NULL) {
    return TCPIP_L05_INVALID_ARGUMENT;
  }
  if (datagram_length > (size_t)UINT16_MAX) {
    return TCPIP_L05_MALFORMED;
  }

  *out_checksum = tcpip_l05_checksum_unchecked(
      source_ipv4, destination_ipv4, datagram, datagram_length);
  return TCPIP_L05_OK;
}
