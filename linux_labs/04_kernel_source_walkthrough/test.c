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
    TCPIP_EXPECT_TRUE(ctx, symbol->source_path != NULL && symbol->source_path[0] != '\0');
    TCPIP_EXPECT_TRUE(ctx, symbol->source_line > 0U);
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
      TCPIP_LINUX_L04_NF_INET_PRE_ROUTING,
      TCPIP_LINUX_L04_IP_RCV_FINISH,
      TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF,
      TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW,
      TCPIP_LINUX_L04_DST_INPUT,
      TCPIP_LINUX_L04_IP_LOCAL_DELIVER,
      TCPIP_LINUX_L04_NF_INET_LOCAL_IN,
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
      TCPIP_LINUX_L04_INET_QUEUE_XMIT,
      TCPIP_LINUX_L04_IP_QUEUE_XMIT,
      TCPIP_LINUX_L04___IP_QUEUE_XMIT,
      TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW,
      TCPIP_LINUX_L04_IP_LOCAL_OUT,
      TCPIP_LINUX_L04_IP_OUTPUT,
      TCPIP_LINUX_L04_IP_FINISH_OUTPUT,
      TCPIP_LINUX_L04___DEV_QUEUE_XMIT};
  size_t count = 0U;
  const tcpip_linux_l04_symbol_id *path = tcpip_linux_l04_ingress_path(&count);
  TCPIP_EXPECT_SIZE(ctx, count, TCPIP_LINUX_L04_INGRESS_COUNT);
  TCPIP_EXPECT_SIZE(ctx, count, sizeof(expected_ingress) / sizeof(expected_ingress[0]));
  TCPIP_EXPECT_BYTES(
      ctx, (const uint8_t *)path, count * sizeof(*path),
      (const uint8_t *)expected_ingress, sizeof(expected_ingress));
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l04_validate_path(path, count), TCPIP_LINUX_L04_OK);
  path = tcpip_linux_l04_egress_path(&count);
  TCPIP_EXPECT_SIZE(ctx, count, TCPIP_LINUX_L04_EGRESS_COUNT);
  TCPIP_EXPECT_SIZE(ctx, count, sizeof(expected_egress) / sizeof(expected_egress[0]));
  TCPIP_EXPECT_BYTES(
      ctx, (const uint8_t *)path, count * sizeof(*path),
      (const uint8_t *)expected_egress, sizeof(expected_egress));
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l04_validate_path(path, count), TCPIP_LINUX_L04_OK);
}

static void tcpip_linux_l04_test_reject_paths(tcpip_test_context *ctx) {
  const tcpip_linux_l04_symbol_id reversed[] = {
      TCPIP_LINUX_L04_IP_RCV_FINISH, TCPIP_LINUX_L04_NF_INET_PRE_ROUTING};
  const tcpip_linux_l04_symbol_id skipped_ingress_hook[] = {
      TCPIP_LINUX_L04_IP_RCV, TCPIP_LINUX_L04_IP_RCV_FINISH};
  const tcpip_linux_l04_symbol_id skipped_egress_queue[] = {
      TCPIP_LINUX_L04_TCP_TRANSMIT_SKB, TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW};
  const tcpip_linux_l04_symbol_id unknown[] = {
      TCPIP_LINUX_L04_IP_RCV, (tcpip_linux_l04_symbol_id)999};
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(reversed, 2U), TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(skipped_ingress_hook, 2U),
      TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(skipped_egress_queue, 2U),
      TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(unknown, 2U), TCPIP_LINUX_L04_NOT_FOUND);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(NULL, 0U), TCPIP_LINUX_L04_INVALID_ARGUMENT);
}

static void tcpip_linux_l04_test_urls(tcpip_test_context *ctx) {
  char url[256];
  char small[8];
  char invalid[8];
  size_t written = 0U;
  size_t required = 0U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, url, sizeof(url), &written),
      TCPIP_LINUX_L04_OK);
  TCPIP_EXPECT_CSTR(
      ctx, url,
      "https://github.com/torvalds/linux/blob/v6.6/net/ipv4/tcp_input.c#L3784");
  TCPIP_EXPECT_SIZE(ctx, written, strlen(url));
  TCPIP_EXPECT_TRUE(ctx, strncmp(url, "https://", 8U) == 0);
  TCPIP_EXPECT_TRUE(ctx, strstr(url, "/v6.6/") != NULL);

  memset(small, 'x', sizeof(small));
  required = 999U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, small, sizeof(small), &required),
      TCPIP_LINUX_L04_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, required, strlen(url));
  TCPIP_EXPECT_TRUE(ctx, small[0] == '\0');
  TCPIP_EXPECT_TRUE(ctx, small[1] == 'x');

  required = 999U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, NULL, 0U, &required),
      TCPIP_LINUX_L04_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, required, strlen(url));

  memset(invalid, 'x', sizeof(invalid));
  written = 999U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          (tcpip_linux_l04_symbol_id)999, invalid, sizeof(invalid), &written),
      TCPIP_LINUX_L04_NOT_FOUND);
  TCPIP_EXPECT_SIZE(ctx, written, 0U);
  TCPIP_EXPECT_TRUE(ctx, invalid[0] == '\0');

  memset(invalid, 'x', sizeof(invalid));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, invalid, sizeof(invalid), NULL),
      TCPIP_LINUX_L04_INVALID_ARGUMENT);
  TCPIP_EXPECT_TRUE(ctx, invalid[0] == '\0');

  written = 999U;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_format_source_url(
          TCPIP_LINUX_L04_TCP_ACK, NULL, 1U, &written),
      TCPIP_LINUX_L04_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(ctx, written, 0U);
}

static void tcpip_linux_l04_test_verified_anchors(tcpip_test_context *ctx) {
  struct expected_anchor {
    tcpip_linux_l04_symbol_id id;
    const char *path;
    unsigned line;
  };
  const struct expected_anchor anchors[] = {
      {TCPIP_LINUX_L04_IP_RCV, "net/ipv4/ip_input.c", 560U},
      {TCPIP_LINUX_L04_NF_INET_PRE_ROUTING, "net/ipv4/ip_input.c", 569U},
      {TCPIP_LINUX_L04_IP_RCV_FINISH, "net/ipv4/ip_input.c", 435U},
      {TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF, "net/ipv4/route.c", 2487U},
      {TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW, "net/ipv4/route.c", 2223U},
      {TCPIP_LINUX_L04_DST_INPUT, "include/net/dst.h", 466U},
      {TCPIP_LINUX_L04_IP_LOCAL_DELIVER, "net/ipv4/ip_input.c", 242U},
      {TCPIP_LINUX_L04_NF_INET_LOCAL_IN, "net/ipv4/ip_input.c", 254U},
      {TCPIP_LINUX_L04_IP_LOCAL_DELIVER_FINISH, "net/ipv4/ip_input.c", 227U},
      {TCPIP_LINUX_L04_TCP_TRANSMIT_SKB, "net/ipv4/tcp_output.c", 1430U},
      {TCPIP_LINUX_L04_INET_QUEUE_XMIT, "net/ipv4/tcp_output.c", 1415U},
      {TCPIP_LINUX_L04_IP_QUEUE_XMIT, "net/ipv4/ip_output.c", 545U},
      {TCPIP_LINUX_L04___IP_QUEUE_XMIT, "net/ipv4/ip_output.c", 454U},
      {TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW, "net/ipv4/route.c", 2869U}};
  size_t index;
  for (index = 0U; index < sizeof(anchors) / sizeof(anchors[0]); index += 1U) {
    const tcpip_linux_l04_symbol *symbol = NULL;
    TCPIP_EXPECT_U32(
        ctx, tcpip_linux_l04_symbol_by_id(anchors[index].id, &symbol), TCPIP_LINUX_L04_OK);
    TCPIP_EXPECT_TRUE(ctx, symbol != NULL);
    if (symbol != NULL) {
      TCPIP_EXPECT_CSTR(ctx, symbol->source_path, anchors[index].path);
      TCPIP_EXPECT_U32(ctx, symbol->source_line, anchors[index].line);
    }
  }
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
  tcpip_linux_l04_test_verified_anchors(&ctx);
  tcpip_linux_l04_test_observation_bridge(&ctx);
  return tcpip_test_finish(&ctx);
}
