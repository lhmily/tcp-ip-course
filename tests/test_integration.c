#include "tcpip/test.h"
#include "../lessons/01_bytes_addressing_checksum/lesson.h"
#include "../lessons/02_ethernet_arp/lesson.h"
#include "../lessons/03_ipv4_packets/lesson.h"
#include "../lessons/05_udp/lesson.h"
#include "../lessons/09_dns/lesson.h"
#include "../lessons/11_routing_nat/lesson.h"
#include "../lessons/12_diagnostics_integration/lesson.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void put_u16(uint8_t *out, uint16_t value) {
  out[0] = (uint8_t)(value >> 8U);
  out[1] = (uint8_t)(value & UINT16_C(0xff));
}

static size_t make_dns_frame(uint8_t *frame, size_t capacity) {
  static const uint8_t destination_mac[6] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};
  static const uint8_t source_mac[6] = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};
  static const uint8_t source_ip[4] = {192U, 0U, 2U, 10U};
  static const uint8_t destination_ip[4] = {198U, 51U, 100U, 53U};
  uint8_t dns[256];
  uint8_t udp[sizeof(dns) + 8U];
  uint8_t ip[20U + sizeof(udp)];
  tcpip_l03_ipv4_header_fields ip_fields = {0};
  size_t dns_length = 0U;
  size_t udp_length = 0U;
  size_t ip_header_length = 0U;
  size_t ip_length;
  size_t frame_length;
  uint16_t ip_checksum = 0U;

  if (frame == NULL ||
      tcpip_l09_build_query(UINT16_C(0x1234), "example.com", 11U, UINT16_C(1),
                            dns, sizeof(dns), &dns_length) != TCPIP_L09_OK ||
      tcpip_l05_build_datagram(source_ip, sizeof(source_ip), destination_ip,
                               sizeof(destination_ip), UINT16_C(53000), UINT16_C(53),
                               dns, dns_length, udp, sizeof(udp), &udp_length) != TCPIP_L05_OK) {
    return 0U;
  }

  ip_length = 20U + udp_length;
  frame_length = TCPIP_L02_ETHERNET_HEADER_LENGTH + ip_length;
  if (capacity < frame_length || ip_length > UINT16_MAX) {
    return 0U;
  }

  ip_fields.identification = UINT16_C(0x4567);
  ip_fields.flags_fragment = UINT16_C(0x4000);
  ip_fields.ttl = UINT8_C(64);
  ip_fields.protocol = UINT8_C(17);
  memcpy(ip_fields.source, source_ip, sizeof(source_ip));
  memcpy(ip_fields.destination, destination_ip, sizeof(destination_ip));
  if (tcpip_l03_build_header(&ip_fields, ip, sizeof(ip), &ip_header_length) !=
          TCPIP_L03_OK ||
      ip_header_length != TCPIP_L03_MIN_HEADER_LENGTH) {
    return 0U;
  }
  put_u16(ip + 2U, (uint16_t)ip_length);
  ip[10U] = 0U;
  ip[11U] = 0U;
  if (tcpip_l01_checksum16(ip, ip_header_length, &ip_checksum) != TCPIP_L01_OK) {
    return 0U;
  }
  put_u16(ip + 10U, ip_checksum);
  memcpy(ip + ip_header_length, udp, udp_length);

  memcpy(frame, destination_mac, sizeof(destination_mac));
  memcpy(frame + 6U, source_mac, sizeof(source_mac));
  put_u16(frame + 12U, UINT16_C(0x0800));
  memcpy(frame + TCPIP_L02_ETHERNET_HEADER_LENGTH, ip, ip_length);
  return frame_length;
}

int main(void) {
  tcpip_test_context test;
  uint8_t frame[512];
  size_t frame_length;
  tcpip_l02_ethernet_frame ethernet;
  tcpip_l03_ipv4_packet ipv4;
  tcpip_l05_datagram udp;
  tcpip_l11_route routes[2] = {
      {{0U, 0U, 0U, 0U}, 0U, {192U, 0U, 2U, 1U}, 1U, 100U},
      {{198U, 51U, 100U, 0U}, 24U, {192U, 0U, 2U, 53U}, 2U, 10U},
  };
  static const uint8_t destination_ip[4] = {198U, 51U, 100U, 53U};
  size_t route_index = SIZE_MAX;
  uint16_t udp_checksum = UINT16_MAX;
  tcpip_l12_report report;
  tcpip_l12_status status;

  tcpip_test_begin(&test, "native cross-layer integration");
  frame_length = make_dns_frame(frame, sizeof(frame));
  TCPIP_EXPECT_TRUE(&test, frame_length > 0U);
  TCPIP_EXPECT_U32(&test,
                   (uint32_t)tcpip_l11_longest_prefix(
                       routes, 2U, destination_ip, &route_index),
                   (uint32_t)TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(&test, route_index, 1U);
  TCPIP_EXPECT_U32(&test, routes[route_index].interface_index, 2U);

  TCPIP_EXPECT_U32(&test,
                   (uint32_t)tcpip_l02_parse_ethernet(frame, frame_length, &ethernet),
                   (uint32_t)TCPIP_L02_OK);
  TCPIP_EXPECT_U32(&test,
                   (uint32_t)tcpip_l03_parse_ipv4(frame + ethernet.payload_offset,
                                                  ethernet.payload_length, &ipv4),
                   (uint32_t)TCPIP_L03_OK);
  TCPIP_EXPECT_U32(&test,
                   (uint32_t)tcpip_l05_parse_datagram(
                       frame + ethernet.payload_offset + ipv4.payload_offset,
                       ipv4.payload_length, &udp),
                   (uint32_t)TCPIP_L05_OK);
  TCPIP_EXPECT_U32(&test,
                   (uint32_t)tcpip_l05_ipv4_checksum(
                       frame + ethernet.payload_offset + 12U, 4U,
                       frame + ethernet.payload_offset + 16U, 4U,
                       frame + ethernet.payload_offset + ipv4.payload_offset,
                       ipv4.payload_length, &udp_checksum),
                   (uint32_t)TCPIP_L05_OK);
  TCPIP_EXPECT_U16(&test, udp_checksum, 0U);

  status = tcpip_l12_diagnose_frame(frame, frame_length, &report);
  TCPIP_EXPECT_U32(&test, (uint32_t)status, (uint32_t)TCPIP_L12_OK);
  TCPIP_EXPECT_U32(&test, report.diagnostics, 0U);
  TCPIP_EXPECT_SIZE(&test, report.layer_count, 4U);
  TCPIP_EXPECT_U32(&test, (uint32_t)report.layers[0], (uint32_t)TCPIP_L12_LAYER_ETHERNET);
  TCPIP_EXPECT_U32(&test, (uint32_t)report.layers[1], (uint32_t)TCPIP_L12_LAYER_IPV4);
  TCPIP_EXPECT_U32(&test, (uint32_t)report.layers[2], (uint32_t)TCPIP_L12_LAYER_UDP);
  TCPIP_EXPECT_U32(&test, (uint32_t)report.layers[3], (uint32_t)TCPIP_L12_LAYER_DNS);

  if (frame_length > 43U) {
    frame[frame_length - 1U] ^= UINT8_C(0x01);
  }
  status = tcpip_l12_diagnose_frame(frame, frame_length, &report);
  TCPIP_EXPECT_U32(&test, (uint32_t)status, (uint32_t)TCPIP_L12_OK);
  TCPIP_EXPECT_TRUE(&test, (report.diagnostics & TCPIP_L12_DIAG_TRANSPORT_CHECKSUM) != 0U);
  TCPIP_EXPECT_TRUE(&test, (report.diagnostics & TCPIP_L12_DIAG_IPV4_CHECKSUM) == 0U);

  return tcpip_test_finish(&test);
}
