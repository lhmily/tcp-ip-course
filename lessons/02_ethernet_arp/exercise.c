#include "lesson.h"

#include <string.h>

tcpip_l02_status tcpip_l02_parse_ethernet(
    const uint8_t *data, size_t data_len, tcpip_l02_ethernet_frame *out_frame) {
  if (out_frame != NULL) {
    memset(out_frame, 0, sizeof(*out_frame));
  }
  if (data == NULL || out_frame == NULL) {
    return TCPIP_L02_INVALID_ARGUMENT;
  }
  if (data_len < TCPIP_L02_ETHERNET_HEADER_LENGTH) {
    return TCPIP_L02_TRUNCATED;
  }

  /* TODO(lesson 02): copy MAC addresses, decode EtherType, and describe the payload span. */
  return TCPIP_L02_TODO;
}

tcpip_l02_status tcpip_l02_parse_arp(
    const uint8_t *data, size_t data_len, tcpip_l02_arp_packet *out_packet) {
  if (out_packet != NULL) {
    memset(out_packet, 0, sizeof(*out_packet));
  }
  if (data == NULL || out_packet == NULL) {
    return TCPIP_L02_INVALID_ARGUMENT;
  }
  if (data_len < TCPIP_L02_ARP_PACKET_LENGTH) {
    return TCPIP_L02_TRUNCATED;
  }

  /* TODO(lesson 02): validate Ethernet/IPv4 ARP widths and opcode before copying fields. */
  return TCPIP_L02_TODO;
}

tcpip_l02_status tcpip_l02_build_arp_request(
    const uint8_t sender_mac[TCPIP_L02_MAC_LENGTH],
    const uint8_t sender_ip[TCPIP_L02_IPV4_LENGTH],
    const uint8_t target_ip[TCPIP_L02_IPV4_LENGTH],
    uint8_t *out_frame,
    size_t out_capacity,
    size_t *out_length) {
  if (out_length != NULL) {
    *out_length = 0U;
  }
  if (sender_mac == NULL || sender_ip == NULL || target_ip == NULL ||
      out_frame == NULL || out_length == NULL) {
    return TCPIP_L02_INVALID_ARGUMENT;
  }
  if (out_capacity < TCPIP_L02_ARP_REQUEST_FRAME_LENGTH) {
    return TCPIP_L02_CAPACITY;
  }

  /* TODO(lesson 02): assemble a 42-byte request locally, then copy it out atomically. */
  return TCPIP_L02_TODO;
}
