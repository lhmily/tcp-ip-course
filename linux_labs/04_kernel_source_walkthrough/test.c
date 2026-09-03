#include "lab.h"

#include <string.h>

#include "tcpip/test.h"

static void tcpip_linux_l04_test_unique_ids(tcpip_test_context *ctx) {
  size_t index;
  size_t previous;
  for (index = 0U; index < TCPIP_LINUX_L04_SYMBOL_COUNT; index += 1U) {
    const tcpip_linux_l04_symbol *symbol = NULL;
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_symbol_by_id((tcpip_linux_l04_symbol_id)index, &symbol),
        TCPIP_LINUX_L04_OK);
    TCPIP_EXPECT_TRUE(ctx, symbol != NULL);
    if (symbol == NULL) {
      continue;
    }
    TCPIP_EXPECT_SIZE(ctx, (size_t)symbol->id, index);
    TCPIP_EXPECT_TRUE(ctx, symbol->key != NULL && symbol->key[0] != '\0');
    TCPIP_EXPECT_TRUE(ctx, symbol->description != NULL && symbol->description[0] != '\0');
    for (previous = 0U; previous < index; previous += 1U) {
      const tcpip_linux_l04_symbol *earlier = NULL;
      (void)tcpip_linux_l04_symbol_by_id((tcpip_linux_l04_symbol_id)previous, &earlier);
      TCPIP_EXPECT_TRUE(ctx, earlier != NULL && strcmp(symbol->key, earlier->key) != 0);
    }
  }
  {
    const tcpip_linux_l04_symbol *symbol = (const tcpip_linux_l04_symbol *)1;
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_symbol_by_id((tcpip_linux_l04_symbol_id)999, &symbol),
        TCPIP_LINUX_L04_NOT_FOUND);
    TCPIP_EXPECT_TRUE(ctx, symbol == NULL);
  }
}

static void tcpip_linux_l04_test_exact_paths(tcpip_test_context *ctx) {
  const tcpip_linux_l04_symbol_id expected_ingress[] = {
      TCPIP_LINUX_L04_NETIF_RECEIVE_SKB,
      TCPIP_LINUX_L04_NETIF_RECEIVE_SKB_ONE_CORE,
      TCPIP_LINUX_L04_IP_RCV,
      TCPIP_LINUX_L04_IP_RCV_FINISH,
      TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF,
      TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW,
      TCPIP_LINUX_L04_IP_LOCAL_DELIVER,
      TCPIP_LINUX_L04_IP_LOCAL_DELIVER_FINISH,
      TCPIP_LINUX_L04_TCP_V4_RCV,
      TCPIP_LINUX_L04_INET_LOOKUP_SKB,
      TCPIP_LINUX_L04_INET_LOOKUP_ESTABLISHED,
      TCPIP_LINUX_L04_TCP_RCV_ESTABLISHED,
      TCPIP_LINUX_L04_TCP_ACK};
  const tcpip_linux_l04_symbol_id expected_egress[] = {
      TCPIP_LINUX_L04_SOCK_SENDMSG,
      TCPIP_LINUX_L04_TCP_SENDMSG,
      TCPIP_LINUX_L04_TCP_SENDMSG_LOCKED,
      TCPIP_LINUX_L04_TCP_WRITE_XMIT,
      TCPIP_LINUX_L04_TCP_TRANSMIT_SKB,
      TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW,
      TCPIP_LINUX_L04_IP_QUEUE_XMIT,
      TCPIP_LINUX_L04___IP_QUEUE_XMIT,
      TCPIP_LINUX_L04_IP_LOCAL_OUT,
      TCPIP_LINUX_L04_IP_OUTPUT,
      TCPIP_LINUX_L04_IP_FINISH_OUTPUT,
      TCPIP_LINUX_L04___DEV_QUEUE_XMIT};
  size_t count = 0U;
  const tcpip_linux_l04_symbol_id *path = tcpip_linux_l04_ingress_path(&count);
  TCPIP_EXPECT_SIZE(ctx, count, TCPIP_LINUX_L04_INGRESS_COUNT);
  TCPIP_EXPECT_BYTES(
      ctx, (const uint8_t *)path, count * sizeof(*path),
      (const uint8_t *)expected_ingress, sizeof(expected_ingress));
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l04_validate_path(path, count), TCPIP_LINUX_L04_OK);
  path = tcpip_linux_l04_egress_path(&count);
  TCPIP_EXPECT_SIZE(ctx, count, TCPIP_LINUX_L04_EGRESS_COUNT);
  TCPIP_EXPECT_BYTES(
      ctx, (const uint8_t *)path, count * sizeof(*path),
      (const uint8_t *)expected_egress, sizeof(expected_egress));
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l04_validate_path(path, count), TCPIP_LINUX_L04_OK);
}

static void tcpip_linux_l04_test_reject_paths(tcpip_test_context *ctx) {
  const tcpip_linux_l04_symbol_id reversed[] = {
      TCPIP_LINUX_L04_IP_RCV_FINISH, TCPIP_LINUX_L04_IP_RCV};
  const tcpip_linux_l04_symbol_id skipped[] = {
      TCPIP_LINUX_L04_IP_RCV, TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF};
  const tcpip_linux_l04_symbol_id unknown[] = {
      TCPIP_LINUX_L04_IP_RCV, (tcpip_linux_l04_symbol_id)999};
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(reversed, 2U), TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(skipped, 2U), TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(unknown, 2U), TCPIP_LINUX_L04_NOT_FOUND);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(NULL, 0U), TCPIP_LINUX_L04_INVALID_ARGUMENT);
}

static void tcpip_linux_l04_test_urls(tcpip_test_context *ctx) {
  char url[256];
  char small[8];
  size_t written = 0U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, url, sizeof(url), &written),
      TCPIP_LINUX_L04_OK);
  TCPIP_EXPECT_CSTR(
      ctx, url,
      "https://github.com/torvalds/linux/blob/v6.6/net/ipv4/tcp_input.c#L3843");
  TCPIP_EXPECT_SIZE(ctx, written, strlen(url));
  TCPIP_EXPECT_TRUE(ctx, strncmp(url, "https://", 8U) == 0);
  TCPIP_EXPECT_TRUE(ctx, strstr(url, "/v6.6/") != NULL);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, small, sizeof(small), &written),
      TCPIP_LINUX_L04_CAPACITY);
  TCPIP_EXPECT_TRUE(ctx, small[sizeof(small) - 1U] == '\0');
}

static void tcpip_linux_l04_test_observation_bridge(tcpip_test_context *ctx) {
  const tcpip_linux_l04_symbol *symbol = NULL;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_symbol_by_id(TCPIP_LINUX_L04_TCP_GET_INFO, &symbol),
      TCPIP_LINUX_L04_OK);
  TCPIP_EXPECT_CSTR(ctx, symbol->key, "tcp_get_info");
  TCPIP_EXPECT_TRUE(ctx, strstr(symbol->description, "TCP_INFO") != NULL);
}

int main(void) {
  tcpip_test_context ctx;
  tcpip_test_begin(&ctx, "linux lab 04 kernel source walkthrough");
  tcpip_linux_l04_test_unique_ids(&ctx);
  tcpip_linux_l04_test_exact_paths(&ctx);
  tcpip_linux_l04_test_reject_paths(&ctx);
  tcpip_linux_l04_test_urls(&ctx);
  tcpip_linux_l04_test_observation_bridge(&ctx);
  return tcpip_test_finish(&ctx);
}
