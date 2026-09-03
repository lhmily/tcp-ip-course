#include "lesson.h"

#include <tcpip/test.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint8_t tcpip_l02_test_sender_mac[TCPIP_L02_MAC_LENGTH] = {
    UINT8_C(0x02), UINT8_C(0x00), UINT8_C(0x5e), UINT8_C(0x10), UINT8_C(0x00), UINT8_C(0x00)};
static const uint8_t tcpip_l02_test_sender_ip[TCPIP_L02_IPV4_LENGTH] = {
    UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(1)};
static const uint8_t tcpip_l02_test_target_ip[TCPIP_L02_IPV4_LENGTH] = {
    UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(99)};
static const uint8_t tcpip_l02_test_frame[TCPIP_L02_ARP_REQUEST_FRAME_LENGTH] = {
    UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff),
    UINT8_C(0x02), UINT8_C(0x00), UINT8_C(0x5e), UINT8_C(0x10), UINT8_C(0x00), UINT8_C(0x00),
    UINT8_C(0x08), UINT8_C(0x06),
    UINT8_C(0x00), UINT8_C(0x01), UINT8_C(0x08), UINT8_C(0x00), UINT8_C(0x06), UINT8_C(0x04),
    UINT8_C(0x00), UINT8_C(0x01),
    UINT8_C(0x02), UINT8_C(0x00), UINT8_C(0x5e), UINT8_C(0x10), UINT8_C(0x00), UINT8_C(0x00),
    UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(1),
    UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00),
    UINT8_C(192), UINT8_C(0), UINT8_C(2), UINT8_C(99)};

static void test_exact_request(tcpip_test_context *ctx) {
  uint8_t output[TCPIP_L02_ARP_REQUEST_FRAME_LENGTH];
  size_t output_length = 99U;

  memset(output, UINT8_C(0xa5), sizeof(output));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_build_arp_request(
          tcpip_l02_test_sender_mac,
          sizeof(tcpip_l02_test_sender_mac),
          tcpip_l02_test_sender_ip,
          sizeof(tcpip_l02_test_sender_ip),
          tcpip_l02_test_target_ip,
          sizeof(tcpip_l02_test_target_ip),
          output,
          sizeof(output),
          &output_length),
      TCPIP_L02_OK);
  TCPIP_EXPECT_SIZE(ctx, output_length, sizeof(tcpip_l02_test_frame));
  TCPIP_EXPECT_BYTES(ctx, output, output_length, tcpip_l02_test_frame, sizeof(tcpip_l02_test_frame));
}

static void test_round_trip_parse(tcpip_test_context *ctx) {
  static const uint8_t broadcast[TCPIP_L02_MAC_LENGTH] = {
      UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff),
      UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff)};
  static const uint8_t zero_mac[TCPIP_L02_MAC_LENGTH] = {0U, 0U, 0U, 0U, 0U, 0U};
  tcpip_l02_ethernet_frame ethernet;
  tcpip_l02_arp_packet arp;

  TCPIP_EXPECT_U32(
      ctx, tcpip_l02_parse_ethernet(tcpip_l02_test_frame, sizeof(tcpip_l02_test_frame), &ethernet), TCPIP_L02_OK);
  TCPIP_EXPECT_BYTES(ctx, ethernet.destination, sizeof(ethernet.destination), broadcast, sizeof(broadcast));
  TCPIP_EXPECT_BYTES(
      ctx, ethernet.source, sizeof(ethernet.source), tcpip_l02_test_sender_mac, sizeof(tcpip_l02_test_sender_mac));
  TCPIP_EXPECT_U16(ctx, ethernet.ether_type, UINT16_C(0x0806));
  TCPIP_EXPECT_SIZE(ctx, ethernet.payload_offset, TCPIP_L02_ETHERNET_HEADER_LENGTH);
  TCPIP_EXPECT_SIZE(ctx, ethernet.payload_length, TCPIP_L02_ARP_PACKET_LENGTH);

  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_parse_arp(
          tcpip_l02_test_frame + ethernet.payload_offset, ethernet.payload_length, &arp),
      TCPIP_L02_OK);
  TCPIP_EXPECT_U16(ctx, arp.hardware_type, UINT16_C(1));
  TCPIP_EXPECT_U16(ctx, arp.protocol_type, UINT16_C(0x0800));
  TCPIP_EXPECT_U32(ctx, arp.hardware_address_length, TCPIP_L02_MAC_LENGTH);
  TCPIP_EXPECT_U32(ctx, arp.protocol_address_length, TCPIP_L02_IPV4_LENGTH);
  TCPIP_EXPECT_U16(ctx, arp.operation, UINT16_C(1));
  TCPIP_EXPECT_BYTES(ctx, arp.sender_mac, sizeof(arp.sender_mac), tcpip_l02_test_sender_mac, sizeof(tcpip_l02_test_sender_mac));
  TCPIP_EXPECT_BYTES(ctx, arp.sender_ip, sizeof(arp.sender_ip), tcpip_l02_test_sender_ip, sizeof(tcpip_l02_test_sender_ip));
  TCPIP_EXPECT_BYTES(ctx, arp.target_mac, sizeof(arp.target_mac), zero_mac, sizeof(zero_mac));
  TCPIP_EXPECT_BYTES(ctx, arp.target_ip, sizeof(arp.target_ip), tcpip_l02_test_target_ip, sizeof(tcpip_l02_test_target_ip));
}

static void test_rejections(tcpip_test_context *ctx) {
  uint8_t malformed[TCPIP_L02_ARP_PACKET_LENGTH];
  uint8_t ethernet_bytes[TCPIP_L02_ETHERNET_HEADER_LENGTH];
  tcpip_l02_ethernet_frame zero_ethernet;
  tcpip_l02_arp_packet zero_arp;
  tcpip_l02_ethernet_frame ethernet;
  tcpip_l02_arp_packet arp;

  memset(&zero_ethernet, 0, sizeof(zero_ethernet));
  memset(&zero_arp, 0, sizeof(zero_arp));
  memset(&ethernet, UINT8_C(0xa5), sizeof(ethernet));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_parse_ethernet(tcpip_l02_test_frame, TCPIP_L02_ETHERNET_HEADER_LENGTH - 1U, &ethernet),
      TCPIP_L02_TRUNCATED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&ethernet,
      sizeof(ethernet),
      (const uint8_t *)&zero_ethernet,
      sizeof(zero_ethernet));

  memcpy(ethernet_bytes, tcpip_l02_test_frame, sizeof(ethernet_bytes));
  ethernet_bytes[12U] = UINT8_C(0x05);
  ethernet_bytes[13U] = UINT8_C(0xdc);
  memset(&ethernet, UINT8_C(0xa5), sizeof(ethernet));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_parse_ethernet(ethernet_bytes, sizeof(ethernet_bytes), &ethernet),
      TCPIP_L02_MALFORMED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&ethernet,
      sizeof(ethernet),
      (const uint8_t *)&zero_ethernet,
      sizeof(zero_ethernet));

  memset(&arp, UINT8_C(0xa5), sizeof(arp));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_parse_arp(tcpip_l02_test_frame + TCPIP_L02_ETHERNET_HEADER_LENGTH,
                          TCPIP_L02_ARP_PACKET_LENGTH - 1U,
                          &arp),
      TCPIP_L02_TRUNCATED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&arp,
      sizeof(arp),
      (const uint8_t *)&zero_arp,
      sizeof(zero_arp));

  memcpy(malformed, tcpip_l02_test_frame + TCPIP_L02_ETHERNET_HEADER_LENGTH, sizeof(malformed));
  malformed[1U] = UINT8_C(2);
  memset(&arp, UINT8_C(0xa5), sizeof(arp));
  TCPIP_EXPECT_U32(ctx, tcpip_l02_parse_arp(malformed, sizeof(malformed), &arp), TCPIP_L02_MALFORMED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&arp,
      sizeof(arp),
      (const uint8_t *)&zero_arp,
      sizeof(zero_arp));

  memcpy(malformed, tcpip_l02_test_frame + TCPIP_L02_ETHERNET_HEADER_LENGTH, sizeof(malformed));
  malformed[2U] = UINT8_C(0x86);
  malformed[3U] = UINT8_C(0xdd);
  memset(&arp, UINT8_C(0xa5), sizeof(arp));
  TCPIP_EXPECT_U32(ctx, tcpip_l02_parse_arp(malformed, sizeof(malformed), &arp), TCPIP_L02_MALFORMED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&arp,
      sizeof(arp),
      (const uint8_t *)&zero_arp,
      sizeof(zero_arp));

  memcpy(malformed, tcpip_l02_test_frame + TCPIP_L02_ETHERNET_HEADER_LENGTH, sizeof(malformed));
  malformed[4U] = UINT8_C(5);
  memset(&arp, UINT8_C(0xa5), sizeof(arp));
  TCPIP_EXPECT_U32(ctx, tcpip_l02_parse_arp(malformed, sizeof(malformed), &arp), TCPIP_L02_MALFORMED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&arp,
      sizeof(arp),
      (const uint8_t *)&zero_arp,
      sizeof(zero_arp));

  memcpy(malformed, tcpip_l02_test_frame + TCPIP_L02_ETHERNET_HEADER_LENGTH, sizeof(malformed));
  malformed[5U] = UINT8_C(16);
  memset(&arp, UINT8_C(0xa5), sizeof(arp));
  TCPIP_EXPECT_U32(ctx, tcpip_l02_parse_arp(malformed, sizeof(malformed), &arp), TCPIP_L02_MALFORMED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&arp,
      sizeof(arp),
      (const uint8_t *)&zero_arp,
      sizeof(zero_arp));

  memcpy(malformed, tcpip_l02_test_frame + TCPIP_L02_ETHERNET_HEADER_LENGTH, sizeof(malformed));
  malformed[6U] = UINT8_C(0);
  malformed[7U] = UINT8_C(3);
  memset(&arp, UINT8_C(0xa5), sizeof(arp));
  TCPIP_EXPECT_U32(ctx, tcpip_l02_parse_arp(malformed, sizeof(malformed), &arp), TCPIP_L02_MALFORMED);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)&arp,
      sizeof(arp),
      (const uint8_t *)&zero_arp,
      sizeof(zero_arp));
}

static void test_builder_rejections_are_atomic(tcpip_test_context *ctx) {
  uint8_t output[TCPIP_L02_ARP_REQUEST_FRAME_LENGTH];
  uint8_t expected[TCPIP_L02_ARP_REQUEST_FRAME_LENGTH];
  size_t output_length = 99U;

  memset(output, UINT8_C(0x5a), sizeof(output));
  memset(expected, UINT8_C(0x5a), sizeof(expected));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_build_arp_request(
          tcpip_l02_test_sender_mac,
          sizeof(tcpip_l02_test_sender_mac),
          tcpip_l02_test_sender_ip,
          sizeof(tcpip_l02_test_sender_ip),
          tcpip_l02_test_target_ip,
          sizeof(tcpip_l02_test_target_ip),
          output,
          sizeof(output) - 1U,
          &output_length),
      TCPIP_L02_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), expected, sizeof(expected));

  output_length = 99U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_build_arp_request(
          tcpip_l02_test_sender_mac,
          TCPIP_L02_MAC_LENGTH - 1U,
          tcpip_l02_test_sender_ip,
          sizeof(tcpip_l02_test_sender_ip),
          tcpip_l02_test_target_ip,
          sizeof(tcpip_l02_test_target_ip),
          output,
          sizeof(output),
          &output_length),
      TCPIP_L02_TRUNCATED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), expected, sizeof(expected));

  output_length = 99U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_build_arp_request(
          tcpip_l02_test_sender_mac,
          sizeof(tcpip_l02_test_sender_mac),
          tcpip_l02_test_sender_ip,
          TCPIP_L02_IPV4_LENGTH - 1U,
          tcpip_l02_test_target_ip,
          sizeof(tcpip_l02_test_target_ip),
          output,
          sizeof(output),
          &output_length),
      TCPIP_L02_TRUNCATED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), expected, sizeof(expected));

  output_length = 99U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_l02_build_arp_request(
          tcpip_l02_test_sender_mac,
          sizeof(tcpip_l02_test_sender_mac),
          tcpip_l02_test_sender_ip,
          sizeof(tcpip_l02_test_sender_ip),
          tcpip_l02_test_target_ip,
          TCPIP_L02_IPV4_LENGTH - 1U,
          output,
          sizeof(output),
          &output_length),
      TCPIP_L02_TRUNCATED);
  TCPIP_EXPECT_SIZE(ctx, output_length, 0U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), expected, sizeof(expected));
}

int main(void) {
  tcpip_test_context ctx;

  tcpip_test_begin(&ctx, "lesson 02: Ethernet and ARP");
  test_exact_request(&ctx);
  test_round_trip_parse(&ctx);
  test_rejections(&ctx);
  test_builder_rejections_are_atomic(&ctx);
  return tcpip_test_finish(&ctx);
}
