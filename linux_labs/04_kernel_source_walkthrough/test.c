#include "lab.h"

#include <stdio.h>
#include <string.h>

#include "tcpip/test.h"

#define TCPIP_LINUX_L04_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

struct tcpip_linux_l04_expected_symbol {
  const char *key;
  const char *name;
  const char *source_path;
  unsigned source_line;
};

static const struct tcpip_linux_l04_expected_symbol tcpip_linux_l04_expected_symbols[] = {
    {"netif_receive_skb", "netif_receive_skb", "net/core/dev.c", 5805U},
    {"__netif_receive_skb_one_core", "__netif_receive_skb_one_core", "net/core/dev.c", 5544U},
    {"ip_rcv", "ip_rcv", "net/ipv4/ip_input.c", 560U},
    {"nf_inet_pre_routing", "NF_INET_PRE_ROUTING", "net/ipv4/ip_input.c", 569U},
    {"ip_rcv_finish", "ip_rcv_finish", "net/ipv4/ip_input.c", 435U},
    {"ip_route_input_noref", "ip_route_input_noref", "net/ipv4/route.c", 2487U},
    {"ip_route_input_slow", "ip_route_input_slow", "net/ipv4/route.c", 2223U},
    {"dst_input", "dst_input", "include/net/dst.h", 466U},
    {"ip_local_deliver", "ip_local_deliver", "net/ipv4/ip_input.c", 242U},
    {"nf_inet_local_in", "NF_INET_LOCAL_IN", "net/ipv4/ip_input.c", 254U},
    {"ip_local_deliver_finish", "ip_local_deliver_finish", "net/ipv4/ip_input.c", 227U},
    {"tcp_v4_rcv", "tcp_v4_rcv", "net/ipv4/tcp_ipv4.c", 1983U},
    {"__inet_lookup_skb", "__inet_lookup_skb", "include/net/inet_hashtables.h", 492U},
    {"__inet_lookup_established", "__inet_lookup_established", "include/net/inet_hashtables.h", 376U},
    {"tcp_rcv_established", "tcp_rcv_established", "net/ipv4/tcp_input.c", 5882U},
    {"tcp_ack", "tcp_ack", "net/ipv4/tcp_input.c", 3784U},
    {"sock_sendmsg", "sock_sendmsg", "net/socket.c", 756U},
    {"tcp_sendmsg", "tcp_sendmsg", "net/ipv4/tcp.c", 1335U},
    {"tcp_sendmsg_locked", "tcp_sendmsg_locked", "net/ipv4/tcp.c", 1037U},
    {"tcp_write_xmit", "tcp_write_xmit", "net/ipv4/tcp_output.c", 2670U},
    {"tcp_transmit_skb", "tcp_transmit_skb", "net/ipv4/tcp_output.c", 1430U},
    {"inet_queue_xmit", "icsk_af_ops->queue_xmit", "net/ipv4/tcp_output.c", 1415U},
    {"ip_route_output_flow", "ip_route_output_flow", "net/ipv4/route.c", 2869U},
    {"ip_queue_xmit", "ip_queue_xmit", "net/ipv4/ip_output.c", 545U},
    {"__ip_queue_xmit", "__ip_queue_xmit", "net/ipv4/ip_output.c", 454U},
    {"ip_local_out", "ip_local_out", "net/ipv4/ip_output.c", 121U},
    {"ip_output", "ip_output", "net/ipv4/ip_output.c", 424U},
    {"ip_finish_output", "ip_finish_output", "net/ipv4/ip_output.c", 314U},
    {"__dev_queue_xmit", "__dev_queue_xmit", "net/core/dev.c", 4277U},
    {"tcp_get_info", "tcp_get_info", "net/ipv4/tcp.c", 3699U}};

static const tcpip_linux_l04_symbol_id tcpip_linux_l04_expected_ingress[] = {
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

static const tcpip_linux_l04_symbol_id tcpip_linux_l04_expected_egress[] = {
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

_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_expected_symbols) ==
        TCPIP_LINUX_L04_SYMBOL_COUNT,
    "test records must cover every generated symbol");
_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_expected_ingress) ==
        TCPIP_LINUX_L04_INGRESS_COUNT,
    "test ingress route must match public metadata");
_Static_assert(
    TCPIP_LINUX_L04_ARRAY_COUNT(tcpip_linux_l04_expected_egress) ==
        TCPIP_LINUX_L04_EGRESS_COUNT,
    "test egress route must match public metadata");

static void tcpip_linux_l04_test_generated_records(tcpip_test_context *ctx) {
  size_t index;
  size_t previous;

  for (index = 0U; index < TCPIP_LINUX_L04_SYMBOL_COUNT; index += 1U) {
    const struct tcpip_linux_l04_expected_symbol *expected =
        &tcpip_linux_l04_expected_symbols[index];
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
    TCPIP_EXPECT_TRUE(ctx, symbol->name != NULL && symbol->name[0] != '\0');
    TCPIP_EXPECT_TRUE(ctx, symbol->source_path != NULL && symbol->source_path[0] != '\0');
    TCPIP_EXPECT_TRUE(ctx, symbol->source_line > 0U);
    TCPIP_EXPECT_TRUE(ctx, symbol->description != NULL && symbol->description[0] != '\0');
    if (symbol->key != NULL) {
      TCPIP_EXPECT_CSTR(ctx, symbol->key, expected->key);
    }
    if (symbol->name != NULL) {
      TCPIP_EXPECT_CSTR(ctx, symbol->name, expected->name);
    }
    if (symbol->source_path != NULL) {
      TCPIP_EXPECT_CSTR(ctx, symbol->source_path, expected->source_path);
    }
    TCPIP_EXPECT_U32(ctx, symbol->source_line, expected->source_line);
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
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_symbol_by_id(TCPIP_LINUX_L04_IP_RCV, NULL),
        TCPIP_LINUX_L04_INVALID_ARGUMENT);
  }
}

static void tcpip_linux_l04_test_routes(tcpip_test_context *ctx) {
  size_t count = 999U;
  const tcpip_linux_l04_symbol_id *path = tcpip_linux_l04_ingress_path(&count);
  TCPIP_EXPECT_TRUE(ctx, path != NULL);
  TCPIP_EXPECT_SIZE(ctx, count, TCPIP_LINUX_L04_INGRESS_COUNT);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)path,
      count * sizeof(*path),
      (const uint8_t *)tcpip_linux_l04_expected_ingress,
      sizeof(tcpip_linux_l04_expected_ingress));
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l04_validate_path(path, count), TCPIP_LINUX_L04_OK);

  count = 999U;
  path = tcpip_linux_l04_egress_path(&count);
  TCPIP_EXPECT_TRUE(ctx, path != NULL);
  TCPIP_EXPECT_SIZE(ctx, count, TCPIP_LINUX_L04_EGRESS_COUNT);
  TCPIP_EXPECT_BYTES(
      ctx,
      (const uint8_t *)path,
      count * sizeof(*path),
      (const uint8_t *)tcpip_linux_l04_expected_egress,
      sizeof(tcpip_linux_l04_expected_egress));
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l04_validate_path(path, count), TCPIP_LINUX_L04_OK);

  TCPIP_EXPECT_TRUE(ctx, tcpip_linux_l04_ingress_path(NULL) == NULL);
  TCPIP_EXPECT_TRUE(ctx, tcpip_linux_l04_egress_path(NULL) == NULL);
}

static void tcpip_linux_l04_test_edge_semantics(tcpip_test_context *ctx) {
  const tcpip_linux_l04_symbol_id direct_local_output[] = {
      TCPIP_LINUX_L04___IP_QUEUE_XMIT, TCPIP_LINUX_L04_IP_LOCAL_OUT};
  const tcpip_linux_l04_symbol_id reversed[] = {
      TCPIP_LINUX_L04_IP_RCV_FINISH, TCPIP_LINUX_L04_NF_INET_PRE_ROUTING};
  const tcpip_linux_l04_symbol_id skipped_ingress_hook[] = {
      TCPIP_LINUX_L04_IP_RCV, TCPIP_LINUX_L04_IP_RCV_FINISH};
  const tcpip_linux_l04_symbol_id skipped_egress_queue[] = {
      TCPIP_LINUX_L04_TCP_TRANSMIT_SKB, TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW};
  const tcpip_linux_l04_symbol_id cross_route[] = {
      TCPIP_LINUX_L04_TCP_ACK, TCPIP_LINUX_L04_SOCK_SENDMSG};
  const tcpip_linux_l04_symbol_id unknown[] = {
      TCPIP_LINUX_L04_IP_RCV, (tcpip_linux_l04_symbol_id)999};
  size_t index;

  for (index = 1U; index < TCPIP_LINUX_L04_INGRESS_COUNT; index += 1U) {
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_validate_path(&tcpip_linux_l04_expected_ingress[index - 1U], 2U),
        TCPIP_LINUX_L04_OK);
  }
  for (index = 1U; index < TCPIP_LINUX_L04_EGRESS_COUNT; index += 1U) {
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_validate_path(&tcpip_linux_l04_expected_egress[index - 1U], 2U),
        TCPIP_LINUX_L04_OK);
  }
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_validate_path(direct_local_output, TCPIP_LINUX_L04_ARRAY_COUNT(direct_local_output)),
      TCPIP_LINUX_L04_OK);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_validate_path(tcpip_linux_l04_expected_ingress, 1U),
      TCPIP_LINUX_L04_OK);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(reversed, 2U), TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_validate_path(skipped_ingress_hook, 2U),
      TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_validate_path(skipped_egress_queue, 2U),
      TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(cross_route, 2U), TCPIP_LINUX_L04_INVALID_PATH);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(unknown, 2U), TCPIP_LINUX_L04_NOT_FOUND);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(NULL, 0U), TCPIP_LINUX_L04_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l04_validate_path(NULL, 1U), TCPIP_LINUX_L04_INVALID_ARGUMENT);
}

static void tcpip_linux_l04_test_all_urls(tcpip_test_context *ctx) {
  char url[256];
  char expected_url[256];
  size_t index;

  for (index = 0U; index < TCPIP_LINUX_L04_SYMBOL_COUNT; index += 1U) {
    const struct tcpip_linux_l04_expected_symbol *expected =
        &tcpip_linux_l04_expected_symbols[index];
    size_t written = 999U;
    int expected_length = snprintf(
        expected_url,
        sizeof(expected_url),
        "https://github.com/torvalds/linux/blob/%s/%s#L%u",
        TCPIP_LINUX_L04_VERSION,
        expected->source_path,
        expected->source_line);
    TCPIP_EXPECT_TRUE(ctx, expected_length > 0 && (size_t)expected_length < sizeof(expected_url));
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_format_source_url(
            (tcpip_linux_l04_symbol_id)index, url, sizeof(url), &written),
        TCPIP_LINUX_L04_OK);
    TCPIP_EXPECT_CSTR(ctx, url, expected_url);
    TCPIP_EXPECT_SIZE(ctx, written, strlen(expected_url));
    TCPIP_EXPECT_TRUE(ctx, strncmp(url, "https://", 8U) == 0);
    TCPIP_EXPECT_TRUE(ctx, strstr(url, "/v6.6/") != NULL);
  }

  {
    char small[8];
    char invalid[8];
    size_t required = 999U;
    size_t written = 999U;
    const size_t tcp_ack_length = strlen(
        "https://github.com/torvalds/linux/blob/v6.6/net/ipv4/tcp_input.c#L3784");

    memset(small, 'x', sizeof(small));
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_format_source_url(
            TCPIP_LINUX_L04_TCP_ACK, small, sizeof(small), &required),
        TCPIP_LINUX_L04_CAPACITY);
    TCPIP_EXPECT_SIZE(ctx, required, tcp_ack_length);
    TCPIP_EXPECT_TRUE(ctx, small[0] == '\0');
    TCPIP_EXPECT_TRUE(ctx, small[1] == 'x');

    required = 999U;
    TCPIP_EXPECT_U32(
        ctx,
        tcpip_linux_l04_format_source_url(
            TCPIP_LINUX_L04_TCP_ACK, NULL, 0U, &required),
        TCPIP_LINUX_L04_CAPACITY);
    TCPIP_EXPECT_SIZE(ctx, required, tcp_ack_length);

    memset(invalid, 'x', sizeof(invalid));
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
}

static void tcpip_linux_l04_test_observation_bridge(tcpip_test_context *ctx) {
  const tcpip_linux_l04_symbol *symbol = NULL;
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l04_symbol_by_id(TCPIP_LINUX_L04_TCP_GET_INFO, &symbol),
      TCPIP_LINUX_L04_OK);
  TCPIP_EXPECT_TRUE(ctx, symbol != NULL);
  if (symbol != NULL) {
    TCPIP_EXPECT_CSTR(ctx, symbol->key, "tcp_get_info");
    TCPIP_EXPECT_TRUE(ctx, strstr(symbol->description, "TCP_INFO") != NULL);
  }
}

int main(void) {
  tcpip_test_context ctx;
  tcpip_test_begin(&ctx, "linux lab 04 kernel source walkthrough");
  tcpip_linux_l04_test_generated_records(&ctx);
  tcpip_linux_l04_test_routes(&ctx);
  tcpip_linux_l04_test_edge_semantics(&ctx);
  tcpip_linux_l04_test_all_urls(&ctx);
  tcpip_linux_l04_test_observation_bridge(&ctx);
  return tcpip_test_finish(&ctx);
}
