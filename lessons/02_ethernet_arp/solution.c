#include "lesson.h"

#include <string.h>

static uint16_t tcpip_l02_read_be16_at(const uint8_t *data, size_t offset) {
  return (uint16_t)(((uint16_t)data[offset] << 8U) | (uint16_t)data[offset + 1U]);
}

static void tcpip_l02_write_be16_at(uint8_t *data, size_t offset, uint16_t value) {
  data[offset] = (uint8_t)(value >> 8U);
  data[offset + 1U] = (uint8_t)(value & UINT16_C(0x00ff));
}

tcpip_l02_status tcpip_l02_parse_ethernet(
    const uint8_t *data, size_t data_len, tcpip_l02_ethernet_frame *out_frame) {
  tcpip_l02_ethernet_frame parsed;

  if (out_frame != NULL) {
    memset(out_frame, 0, sizeof(*out_frame));
  }
  if (data == NULL || out_frame == NULL) {
    return TCPIP_L02_INVALID_ARGUMENT;
  }
  if (data_len < TCPIP_L02_ETHERNET_HEADER_LENGTH) {
    return TCPIP_L02_TRUNCATED;
  }

  memset(&parsed, 0, sizeof(parsed));
  memcpy(parsed.destination, data, TCPIP_L02_MAC_LENGTH);
  memcpy(parsed.source, data + TCPIP_L02_MAC_LENGTH, TCPIP_L02_MAC_LENGTH);
  parsed.ether_type = tcpip_l02_read_be16_at(data, 12U);
  if (parsed.ether_type < UINT16_C(0x0600)) {
    return TCPIP_L02_MALFORMED;
  }
  parsed.payload_offset = TCPIP_L02_ETHERNET_HEADER_LENGTH;
  parsed.payload_length = data_len - TCPIP_L02_ETHERNET_HEADER_LENGTH;
  *out_frame = parsed;
  return TCPIP_L02_OK;
}

tcpip_l02_status tcpip_l02_parse_arp(
    const uint8_t *data, size_t data_len, tcpip_l02_arp_packet *out_packet) {
  tcpip_l02_arp_packet parsed;

  if (out_packet != NULL) {
    memset(out_packet, 0, sizeof(*out_packet));
  }
  if (data == NULL || out_packet == NULL) {
    return TCPIP_L02_INVALID_ARGUMENT;
  }
  if (data_len < TCPIP_L02_ARP_PACKET_LENGTH) {
    return TCPIP_L02_TRUNCATED;
  }

  memset(&parsed, 0, sizeof(parsed));
  parsed.hardware_type = tcpip_l02_read_be16_at(data, 0U);
  parsed.protocol_type = tcpip_l02_read_be16_at(data, 2U);
  parsed.hardware_address_length = data[4U];
  parsed.protocol_address_length = data[5U];
  parsed.operation = tcpip_l02_read_be16_at(data, 6U);

  if (parsed.hardware_type != UINT16_C(1) ||
      parsed.protocol_type != UINT16_C(0x0800) ||
      parsed.hardware_address_length != TCPIP_L02_MAC_LENGTH ||
      parsed.protocol_address_length != TCPIP_L02_IPV4_LENGTH ||
      (parsed.operation != UINT16_C(1) && parsed.operation != UINT16_C(2))) {
    return TCPIP_L02_MALFORMED;
  }

  memcpy(parsed.sender_mac, data + 8U, TCPIP_L02_MAC_LENGTH);
  memcpy(parsed.sender_ip, data + 14U, TCPIP_L02_IPV4_LENGTH);
  memcpy(parsed.target_mac, data + 18U, TCPIP_L02_MAC_LENGTH);
  memcpy(parsed.target_ip, data + 24U, TCPIP_L02_IPV4_LENGTH);
  *out_packet = parsed;
  return TCPIP_L02_OK;
}

tcpip_l02_status tcpip_l02_build_arp_request(
    const uint8_t *sender_mac,
    size_t sender_mac_length,
    const uint8_t *sender_ip,
    size_t sender_ip_length,
    const uint8_t *target_ip,
    size_t target_ip_length,
    uint8_t *out_frame,
    size_t out_capacity,
    size_t *out_length) {
  uint8_t frame[TCPIP_L02_ARP_REQUEST_FRAME_LENGTH] = {0U};
  static const uint8_t broadcast[TCPIP_L02_MAC_LENGTH] = {
      UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff),
      UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff)};

  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (sender_mac == NULL || sender_ip == NULL || target_ip == NULL ||
      out_frame == NULL || out_length == NULL) {
    return TCPIP_L02_INVALID_ARGUMENT;
  }
  if (sender_mac_length < TCPIP_L02_MAC_LENGTH ||
      sender_ip_length < TCPIP_L02_IPV4_LENGTH ||
      target_ip_length < TCPIP_L02_IPV4_LENGTH) {
    return TCPIP_L02_TRUNCATED;
  }
  if (out_capacity < TCPIP_L02_ARP_REQUEST_FRAME_LENGTH) {
    return TCPIP_L02_CAPACITY;
  }

  memcpy(frame, broadcast, sizeof(broadcast));
  memcpy(frame + 6U, sender_mac, TCPIP_L02_MAC_LENGTH);
  tcpip_l02_write_be16_at(frame, 12U, UINT16_C(0x0806));

  tcpip_l02_write_be16_at(frame, 14U, UINT16_C(1));
  tcpip_l02_write_be16_at(frame, 16U, UINT16_C(0x0800));
  frame[18U] = TCPIP_L02_MAC_LENGTH;
  frame[19U] = TCPIP_L02_IPV4_LENGTH;
  tcpip_l02_write_be16_at(frame, 20U, UINT16_C(1));
  memcpy(frame + 22U, sender_mac, TCPIP_L02_MAC_LENGTH);
  memcpy(frame + 28U, sender_ip, TCPIP_L02_IPV4_LENGTH);
  memcpy(frame + 38U, target_ip, TCPIP_L02_IPV4_LENGTH);

  memcpy(out_frame, frame, sizeof(frame));
  *out_length = sizeof(frame);
  return TCPIP_L02_OK;
}
