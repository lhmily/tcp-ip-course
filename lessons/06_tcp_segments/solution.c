#include "lesson.h"

#include <limits.h>
#include <string.h>

#define TCPIP_L06_TCP_MIN_HEADER_LENGTH 20U
#define TCPIP_L06_TCP_MAX_OPTIONS_LENGTH 40U
#define TCPIP_L06_TCP_PROTOCOL 6U
#define TCPIP_L06_SERIAL_HALF_RANGE UINT32_C(0x80000000)

static uint16_t tcpip_l06_read_u16(const uint8_t *bytes) {
  return (uint16_t)(((uint16_t)bytes[0] << 8U) | (uint16_t)bytes[1]);
}

static uint32_t tcpip_l06_read_u32(const uint8_t *bytes) {
  return ((uint32_t)bytes[0] << 24U) | ((uint32_t)bytes[1] << 16U) |
         ((uint32_t)bytes[2] << 8U) | (uint32_t)bytes[3];
}

static void tcpip_l06_write_u16(uint8_t *bytes, uint16_t value) {
  bytes[0] = (uint8_t)(value >> 8U);
  bytes[1] = (uint8_t)value;
}

static void tcpip_l06_write_u32(uint8_t *bytes, uint32_t value) {
  bytes[0] = (uint8_t)(value >> 24U);
  bytes[1] = (uint8_t)(value >> 16U);
  bytes[2] = (uint8_t)(value >> 8U);
  bytes[3] = (uint8_t)value;
}

static int tcpip_l06_valid_ipv4_span(const uint8_t *address, size_t length) {
  return address != NULL && length == 4U;
}

static uint32_t tcpip_l06_add_word(uint32_t sum, uint16_t word) {
  sum += (uint32_t)word;
  return (sum & UINT32_C(0xffff)) + (sum >> 16U);
}

static uint32_t tcpip_l06_add_bytes(uint32_t sum, const uint8_t *bytes, size_t length) {
  size_t index = 0U;

  while (length - index >= 2U) {
    sum = tcpip_l06_add_word(sum, tcpip_l06_read_u16(bytes + index));
    index += 2U;
  }
  if (index < length) {
    sum = tcpip_l06_add_word(sum, (uint16_t)((uint16_t)bytes[index] << 8U));
  }
  return sum;
}

static uint16_t tcpip_l06_finish_checksum(uint32_t sum) {
  while ((sum >> 16U) != 0U) {
    sum = (sum & UINT32_C(0xffff)) + (sum >> 16U);
  }
  return (uint16_t)~(uint16_t)sum;
}

static uint16_t tcpip_l06_checksum_unchecked(
    const uint8_t source_ipv4[4],
    const uint8_t destination_ipv4[4],
    const uint8_t *segment,
    size_t segment_length) {
  uint32_t sum = 0U;

  sum = tcpip_l06_add_bytes(sum, source_ipv4, 4U);
  sum = tcpip_l06_add_bytes(sum, destination_ipv4, 4U);
  sum = tcpip_l06_add_word(sum, (uint16_t)TCPIP_L06_TCP_PROTOCOL);
  sum = tcpip_l06_add_word(sum, (uint16_t)segment_length);
  sum = tcpip_l06_add_bytes(sum, segment, segment_length);
  return tcpip_l06_finish_checksum(sum);
}

tcpip_l06_status tcpip_l06_parse_segment(
    const uint8_t *segment,
    size_t segment_length,
    tcpip_l06_segment *out_segment) {
  uint8_t data_offset;
  size_t header_length;

  if (out_segment == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  memset(out_segment, 0, sizeof(*out_segment));
  if (segment == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  if (segment_length < TCPIP_L06_TCP_MIN_HEADER_LENGTH) {
    return TCPIP_L06_TRUNCATED;
  }

  data_offset = (uint8_t)(segment[12] >> 4U);
  if (data_offset < 5U || (segment[12] & UINT8_C(0x0e)) != 0U) {
    return TCPIP_L06_MALFORMED;
  }
  header_length = (size_t)data_offset * 4U;
  if (segment_length < header_length) {
    return TCPIP_L06_TRUNCATED;
  }

  out_segment->source_port = tcpip_l06_read_u16(segment);
  out_segment->destination_port = tcpip_l06_read_u16(segment + 2U);
  out_segment->sequence_number = tcpip_l06_read_u32(segment + 4U);
  out_segment->acknowledgment_number = tcpip_l06_read_u32(segment + 8U);
  out_segment->data_offset = data_offset;
  out_segment->flags = (uint16_t)(((uint16_t)(segment[12] & UINT8_C(0x01)) << 8U) |
                                  (uint16_t)segment[13]);
  out_segment->window = tcpip_l06_read_u16(segment + 14U);
  out_segment->checksum = tcpip_l06_read_u16(segment + 16U);
  out_segment->urgent_pointer = tcpip_l06_read_u16(segment + 18U);
  out_segment->options_offset = TCPIP_L06_TCP_MIN_HEADER_LENGTH;
  out_segment->options_length = header_length - TCPIP_L06_TCP_MIN_HEADER_LENGTH;
  out_segment->payload_offset = header_length;
  out_segment->payload_length = segment_length - header_length;
  return TCPIP_L06_OK;
}

tcpip_l06_status tcpip_l06_build_segment(
    const tcpip_l06_segment_fields *fields,
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *destination,
    size_t destination_capacity,
    size_t *out_segment_length) {
  uint8_t source_copy[4];
  uint8_t destination_copy[4];
  size_t header_length;
  size_t segment_length;
  uint16_t checksum;

  if (out_segment_length == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  *out_segment_length = 0U;
  if (fields == NULL || destination == NULL ||
      (payload == NULL && payload_length != 0U)) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  if (!tcpip_l06_valid_ipv4_span(fields->source_ipv4, fields->source_ipv4_length) ||
      !tcpip_l06_valid_ipv4_span(
          fields->destination_ipv4, fields->destination_ipv4_length) ||
      (fields->options == NULL && fields->options_length != 0U)) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  if (fields->options_length > TCPIP_L06_TCP_MAX_OPTIONS_LENGTH ||
      fields->options_length % 4U != 0U ||
      fields->flags > UINT16_C(0x01ff)) {
    return TCPIP_L06_MALFORMED;
  }

  header_length = TCPIP_L06_TCP_MIN_HEADER_LENGTH + fields->options_length;
  if (payload_length > (size_t)UINT16_MAX - header_length) {
    return TCPIP_L06_MALFORMED;
  }
  segment_length = header_length + payload_length;
  if (destination_capacity < segment_length) {
    return TCPIP_L06_CAPACITY;
  }

  memcpy(source_copy, fields->source_ipv4, sizeof(source_copy));
  memcpy(destination_copy, fields->destination_ipv4, sizeof(destination_copy));
  if (payload_length != 0U) {
    memmove(destination + header_length, payload, payload_length);
  }
  if (fields->options_length != 0U) {
    memmove(
        destination + TCPIP_L06_TCP_MIN_HEADER_LENGTH,
        fields->options,
        fields->options_length);
  }
  tcpip_l06_write_u16(destination, fields->source_port);
  tcpip_l06_write_u16(destination + 2U, fields->destination_port);
  tcpip_l06_write_u32(destination + 4U, fields->sequence_number);
  tcpip_l06_write_u32(destination + 8U, fields->acknowledgment_number);
  destination[12] = (uint8_t)(((uint8_t)(header_length / 4U) << 4U) |
                              (uint8_t)((fields->flags >> 8U) & UINT16_C(0x0001)));
  destination[13] = (uint8_t)fields->flags;
  tcpip_l06_write_u16(destination + 14U, fields->window);
  tcpip_l06_write_u16(destination + 16U, 0U);
  tcpip_l06_write_u16(destination + 18U, fields->urgent_pointer);

  checksum = tcpip_l06_checksum_unchecked(
      source_copy, destination_copy, destination, segment_length);
  tcpip_l06_write_u16(destination + 16U, checksum);
  *out_segment_length = segment_length;
  return TCPIP_L06_OK;
}

tcpip_l06_status tcpip_l06_ipv4_checksum(
    const uint8_t *source_ipv4,
    size_t source_ipv4_length,
    const uint8_t *destination_ipv4,
    size_t destination_ipv4_length,
    const uint8_t *segment,
    size_t segment_length,
    uint16_t *out_checksum) {
  if (out_checksum == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  *out_checksum = 0U;
  if (!tcpip_l06_valid_ipv4_span(source_ipv4, source_ipv4_length) ||
      !tcpip_l06_valid_ipv4_span(destination_ipv4, destination_ipv4_length) ||
      segment == NULL) {
    return TCPIP_L06_INVALID_ARGUMENT;
  }
  if (segment_length > (size_t)UINT16_MAX) {
    return TCPIP_L06_MALFORMED;
  }

  *out_checksum = tcpip_l06_checksum_unchecked(
      source_ipv4, destination_ipv4, segment, segment_length);
  return TCPIP_L06_OK;
}

int tcpip_l06_seq_before(uint32_t first, uint32_t second) {
  uint32_t distance = second - first;
  return distance != 0U && distance < TCPIP_L06_SERIAL_HALF_RANGE;
}

uint32_t tcpip_l06_seq_distance(uint32_t first, uint32_t second) {
  return second - first;
}
