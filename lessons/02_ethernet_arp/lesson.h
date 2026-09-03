#ifndef TCPIP_L02_LESSON_H
#define TCPIP_L02_LESSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_L02_MAC_LENGTH 6U
#define TCPIP_L02_IPV4_LENGTH 4U
#define TCPIP_L02_ETHERNET_HEADER_LENGTH 14U
#define TCPIP_L02_ARP_PACKET_LENGTH 28U
#define TCPIP_L02_ARP_REQUEST_FRAME_LENGTH 42U

typedef enum tcpip_l02_status {
  TCPIP_L02_OK = 0,
  TCPIP_L02_INVALID_ARGUMENT,
  TCPIP_L02_TRUNCATED,
  TCPIP_L02_MALFORMED,
  TCPIP_L02_CAPACITY,
  TCPIP_L02_TODO
} tcpip_l02_status;

typedef struct tcpip_l02_ethernet_frame {
  uint8_t destination[TCPIP_L02_MAC_LENGTH];
  uint8_t source[TCPIP_L02_MAC_LENGTH];
  uint16_t ether_type;
  size_t payload_offset;
  size_t payload_length;
} tcpip_l02_ethernet_frame;

typedef struct tcpip_l02_arp_packet {
  uint16_t hardware_type;
  uint16_t protocol_type;
  uint8_t hardware_address_length;
  uint8_t protocol_address_length;
  uint16_t operation;
  uint8_t sender_mac[TCPIP_L02_MAC_LENGTH];
  uint8_t sender_ip[TCPIP_L02_IPV4_LENGTH];
  uint8_t target_mac[TCPIP_L02_MAC_LENGTH];
  uint8_t target_ip[TCPIP_L02_IPV4_LENGTH];
} tcpip_l02_arp_packet;

tcpip_l02_status tcpip_l02_parse_ethernet(
    const uint8_t *data, size_t data_len, tcpip_l02_ethernet_frame *out_frame);

tcpip_l02_status tcpip_l02_parse_arp(
    const uint8_t *data, size_t data_len, tcpip_l02_arp_packet *out_packet);

tcpip_l02_status tcpip_l02_build_arp_request(
    const uint8_t *sender_mac,
    size_t sender_mac_length,
    const uint8_t *sender_ip,
    size_t sender_ip_length,
    const uint8_t *target_ip,
    size_t target_ip_length,
    uint8_t *out_frame,
    size_t out_capacity,
    size_t *out_length);

#ifdef __cplusplus
}
#endif

#endif
