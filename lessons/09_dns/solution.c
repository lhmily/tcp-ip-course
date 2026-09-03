#include "lesson.h"

#include <string.h>

#define TCPIP_L09_DNS_HEADER_SIZE 12U
#define TCPIP_L09_MAX_WIRE_NAME 255U
#define TCPIP_L09_MAX_TEXT_NAME 253U
#define TCPIP_L09_POINTER_SPACE 16384U
#define TCPIP_L09_VISITED_BYTES (TCPIP_L09_POINTER_SPACE / 8U)
#define TCPIP_L09_MAX_POINTER_HOPS 128U

static uint16_t tcpip_l09_read_u16(const uint8_t *input) {
  return (uint16_t)(((uint16_t)input[0] << 8U) | (uint16_t)input[1]);
}

static uint32_t tcpip_l09_read_u32(const uint8_t *input) {
  return ((uint32_t)input[0] << 24U) | ((uint32_t)input[1] << 16U) |
         ((uint32_t)input[2] << 8U) | (uint32_t)input[3];
}

static void tcpip_l09_write_u16(uint8_t *output, uint16_t value) {
  output[0] = (uint8_t)(value >> 8U);
  output[1] = (uint8_t)(value & UINT16_C(0x00ff));
}

static int tcpip_l09_visited_get(const uint8_t *visited, size_t offset) {
  const size_t byte_index = offset / 8U;
  const unsigned bit_index = (unsigned)(offset % 8U);
  return (visited[byte_index] & (uint8_t)(UINT8_C(1) << bit_index)) != 0U;
}

static void tcpip_l09_visited_set(uint8_t *visited, size_t offset) {
  const size_t byte_index = offset / 8U;
  const unsigned bit_index = (unsigned)(offset % 8U);
  visited[byte_index] |= (uint8_t)(UINT8_C(1) << bit_index);
}

static tcpip_l09_status tcpip_l09_decode_name_internal(
    const uint8_t *message,
    size_t message_len,
    size_t offset,
    uint8_t decoded[TCPIP_L09_MAX_TEXT_NAME + 1U],
    size_t *next_offset,
    size_t *decoded_len) {
  uint8_t visited[TCPIP_L09_VISITED_BYTES];
  size_t cursor = offset;
  size_t result_len = 0U;
  size_t expanded_wire_len = 0U;
  size_t pointer_hops = 0U;
  int jumped = 0;

  memset(visited, 0, sizeof(visited));
  for (;;) {
    uint8_t length;

    if (cursor >= message_len) {
      return TCPIP_L09_TRUNCATED;
    }
    length = message[cursor];

    if ((length & UINT8_C(0xc0)) == UINT8_C(0xc0)) {
      size_t target;
      if (message_len - cursor < 2U) {
        return TCPIP_L09_TRUNCATED;
      }
      target = (size_t)(((uint16_t)(length & UINT8_C(0x3f)) << 8U) |
                        (uint16_t)message[cursor + 1U]);
      if (target >= message_len) {
        return TCPIP_L09_MALFORMED;
      }
      if (!jumped) {
        *next_offset = cursor + 2U;
        jumped = 1;
      }
      if (tcpip_l09_visited_get(visited, target)) {
        return TCPIP_L09_MALFORMED;
      }
      tcpip_l09_visited_set(visited, target);
      pointer_hops += 1U;
      if (pointer_hops > TCPIP_L09_MAX_POINTER_HOPS) {
        return TCPIP_L09_MALFORMED;
      }
      cursor = target;
      continue;
    }

    if ((length & UINT8_C(0xc0)) != 0U) {
      return TCPIP_L09_MALFORMED;
    }
    cursor += 1U;
    if (length == 0U) {
      expanded_wire_len += 1U;
      if (expanded_wire_len > TCPIP_L09_MAX_WIRE_NAME) {
        return TCPIP_L09_MALFORMED;
      }
      if (!jumped) {
        *next_offset = cursor;
      }
      decoded[result_len] = 0U;
      *decoded_len = result_len;
      return TCPIP_L09_OK;
    }

    if ((size_t)length > message_len - cursor) {
      return TCPIP_L09_TRUNCATED;
    }
    if (expanded_wire_len > TCPIP_L09_MAX_WIRE_NAME - ((size_t)length + 1U)) {
      return TCPIP_L09_MALFORMED;
    }
    expanded_wire_len += (size_t)length + 1U;
    if (result_len != 0U) {
      if (result_len >= TCPIP_L09_MAX_TEXT_NAME) {
        return TCPIP_L09_MALFORMED;
      }
      decoded[result_len] = (uint8_t)'.';
      result_len += 1U;
    }
    if ((size_t)length > TCPIP_L09_MAX_TEXT_NAME - result_len) {
      return TCPIP_L09_MALFORMED;
    }
    memcpy(decoded + result_len, message + cursor, (size_t)length);
    result_len += (size_t)length;
    cursor += (size_t)length;
  }
}

tcpip_l09_status tcpip_l09_encode_name(
    const char *name,
    size_t name_len,
    uint8_t *out,
    size_t out_capacity,
    size_t *written) {
  size_t encoded_len = 1U;
  size_t label_start = 0U;
  size_t input_end = name_len;
  size_t input_index;
  size_t output_index = 0U;

  if (written == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  *written = 0U;
  if ((name_len != 0U && name == NULL) || (out_capacity != 0U && out == NULL)) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (name_len == 1U && name[0] == (uint8_t)'.') {
    input_end = 0U;
  } else if (name_len != 0U && name[name_len - 1U] == (uint8_t)'.') {
    input_end -= 1U;
  }

  for (input_index = 0U; input_index < input_end; input_index += 1U) {
    if (name[input_index] == 0U) {
      return TCPIP_L09_MALFORMED;
    }
    if (name[input_index] == (uint8_t)'.') {
      const size_t label_len = input_index - label_start;
      if (label_len == 0U || label_len > 63U) {
        return TCPIP_L09_MALFORMED;
      }
      if (encoded_len > TCPIP_L09_MAX_WIRE_NAME - (label_len + 1U)) {
        return TCPIP_L09_MALFORMED;
      }
      encoded_len += label_len + 1U;
      label_start = input_index + 1U;
    }
  }
  if (input_end != 0U) {
    const size_t label_len = input_end - label_start;
    if (label_len == 0U || label_len > 63U) {
      return TCPIP_L09_MALFORMED;
    }
    if (encoded_len > TCPIP_L09_MAX_WIRE_NAME - (label_len + 1U)) {
      return TCPIP_L09_MALFORMED;
    }
    encoded_len += label_len + 1U;
  }
  if (encoded_len > out_capacity) {
    return TCPIP_L09_CAPACITY;
  }

  label_start = 0U;
  for (input_index = 0U; input_index <= input_end; input_index += 1U) {
    if (input_index == input_end || name[input_index] == (uint8_t)'.') {
      const size_t label_len = input_index - label_start;
      if (label_len != 0U) {
        out[output_index] = (uint8_t)label_len;
        output_index += 1U;
        memcpy(out + output_index, name + label_start, label_len);
        output_index += label_len;
      }
      label_start = input_index + 1U;
    }
  }
  out[output_index] = 0U;
  output_index += 1U;
  *written = output_index;
  return TCPIP_L09_OK;
}

tcpip_l09_status tcpip_l09_decode_name(
    const uint8_t *message,
    size_t message_len,
    size_t offset,
    uint8_t *out,
    size_t out_capacity,
    size_t *next_offset,
    size_t *written) {
  uint8_t decoded[TCPIP_L09_MAX_TEXT_NAME + 1U];
  size_t decoded_next = 0U;
  size_t decoded_len = 0U;
  tcpip_l09_status status;

  if (next_offset == NULL || written == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  *next_offset = 0U;
  *written = 0U;
  if (message == NULL || (out_capacity != 0U && out == NULL)) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (offset >= message_len) {
    return TCPIP_L09_TRUNCATED;
  }

  status = tcpip_l09_decode_name_internal(
      message, message_len, offset, decoded, &decoded_next, &decoded_len);
  if (status != TCPIP_L09_OK) {
    return status;
  }
  if (decoded_len + 1U > out_capacity) {
    return TCPIP_L09_CAPACITY;
  }
  memcpy(out, decoded, decoded_len + 1U);
  *next_offset = decoded_next;
  *written = decoded_len;
  return TCPIP_L09_OK;
}

tcpip_l09_status tcpip_l09_build_query(
    uint16_t id,
    const char *name,
    size_t name_len,
    uint16_t qtype,
    uint8_t *out,
    size_t out_capacity,
    size_t *written) {
  uint8_t encoded_name[TCPIP_L09_MAX_WIRE_NAME];
  size_t encoded_len = 0U;
  size_t total_len;
  tcpip_l09_status status;

  if (written == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  *written = 0U;
  if ((name_len != 0U && name == NULL) || (out_capacity != 0U && out == NULL) || qtype == 0U) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }

  status = tcpip_l09_encode_name(
      name, name_len, encoded_name, sizeof(encoded_name), &encoded_len);
  if (status != TCPIP_L09_OK) {
    return status;
  }
  total_len = TCPIP_L09_DNS_HEADER_SIZE + encoded_len + 4U;
  if (total_len > out_capacity) {
    return TCPIP_L09_CAPACITY;
  }

  memset(out, 0, TCPIP_L09_DNS_HEADER_SIZE);
  tcpip_l09_write_u16(out, id);
  tcpip_l09_write_u16(out + 2U, UINT16_C(0x0100));
  tcpip_l09_write_u16(out + 4U, UINT16_C(1));
  memcpy(out + TCPIP_L09_DNS_HEADER_SIZE, encoded_name, encoded_len);
  tcpip_l09_write_u16(out + TCPIP_L09_DNS_HEADER_SIZE + encoded_len, qtype);
  tcpip_l09_write_u16(out + TCPIP_L09_DNS_HEADER_SIZE + encoded_len + 2U, UINT16_C(1));
  *written = total_len;
  return TCPIP_L09_OK;
}

tcpip_l09_status tcpip_l09_parse_message(
    const uint8_t *message,
    size_t message_len,
    tcpip_l09_dns_header *header) {
  uint16_t flags;

  if (header == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  memset(header, 0, sizeof(*header));
  if (message == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  if (message_len < TCPIP_L09_DNS_HEADER_SIZE) {
    return TCPIP_L09_TRUNCATED;
  }

  flags = tcpip_l09_read_u16(message + 2U);
  header->id = tcpip_l09_read_u16(message);
  header->flags = flags;
  header->question_count = tcpip_l09_read_u16(message + 4U);
  header->answer_count = tcpip_l09_read_u16(message + 6U);
  header->authority_count = tcpip_l09_read_u16(message + 8U);
  header->additional_count = tcpip_l09_read_u16(message + 10U);
  header->rcode = (uint8_t)(flags & UINT16_C(0x000f));
  return TCPIP_L09_OK;
}

tcpip_l09_status tcpip_l09_first_a(
    const uint8_t *message,
    size_t message_len,
    uint8_t out[4],
    uint32_t *ttl) {
  tcpip_l09_dns_header header;
  size_t cursor = TCPIP_L09_DNS_HEADER_SIZE;
  uint16_t index;
  uint8_t ignored_name[TCPIP_L09_MAX_TEXT_NAME + 1U];

  if (out == NULL || ttl == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }
  memset(out, 0, 4U);
  *ttl = 0U;
  if (message == NULL) {
    return TCPIP_L09_INVALID_ARGUMENT;
  }

  {
    const tcpip_l09_status status = tcpip_l09_parse_message(message, message_len, &header);
    if (status != TCPIP_L09_OK) {
      return status;
    }
  }
  if ((header.flags & UINT16_C(0x8000)) == 0U ||
      (header.flags & UINT16_C(0x7800)) != 0U ||
      (header.flags & UINT16_C(0x0040)) != 0U) {
    return TCPIP_L09_MALFORMED;
  }
  if ((header.flags & UINT16_C(0x0200)) != 0U) {
    return TCPIP_L09_TRUNCATED;
  }
  if (header.rcode != 0U) {
    return TCPIP_L09_MALFORMED;
  }

  for (index = 0U; index < header.question_count; index += 1U) {
    size_t next = 0U;
    size_t ignored_len = 0U;
    const tcpip_l09_status status = tcpip_l09_decode_name(
        message, message_len, cursor, ignored_name, sizeof(ignored_name), &next, &ignored_len);
    (void)ignored_len;
    if (status != TCPIP_L09_OK) {
      return status;
    }
    cursor = next;
    if (cursor > message_len || message_len - cursor < 4U) {
      return TCPIP_L09_TRUNCATED;
    }
    cursor += 4U;
  }

  for (index = 0U; index < header.answer_count; index += 1U) {
    size_t next = 0U;
    size_t ignored_len = 0U;
    uint16_t type;
    uint16_t record_class;
    uint32_t record_ttl;
    uint16_t data_len;
    const tcpip_l09_status status = tcpip_l09_decode_name(
        message, message_len, cursor, ignored_name, sizeof(ignored_name), &next, &ignored_len);
    (void)ignored_len;
    if (status != TCPIP_L09_OK) {
      return status;
    }
    cursor = next;
    if (cursor > message_len || message_len - cursor < 10U) {
      return TCPIP_L09_TRUNCATED;
    }
    type = tcpip_l09_read_u16(message + cursor);
    record_class = tcpip_l09_read_u16(message + cursor + 2U);
    record_ttl = tcpip_l09_read_u32(message + cursor + 4U);
    data_len = tcpip_l09_read_u16(message + cursor + 8U);
    cursor += 10U;
    if ((size_t)data_len > message_len - cursor) {
      return TCPIP_L09_TRUNCATED;
    }
    if (type == UINT16_C(1) && record_class == UINT16_C(1) && data_len == UINT16_C(4)) {
      memcpy(out, message + cursor, 4U);
      *ttl = record_ttl;
      return TCPIP_L09_OK;
    }
    cursor += (size_t)data_len;
  }

  return TCPIP_L09_MALFORMED;
}
