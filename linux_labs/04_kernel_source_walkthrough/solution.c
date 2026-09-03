#include "lab.h"

#include <stdio.h>
#include <string.h>

typedef struct tcpip_linux_l04_edge {
  tcpip_linux_l04_symbol_id from;
  tcpip_linux_l04_symbol_id to;
} tcpip_linux_l04_edge;

#define L04_SYMBOL(id_value, key_value, name_value, path_value, line_value, description_value) \
  {id_value, key_value, name_value, path_value, line_value, description_value}

static const tcpip_linux_l04_symbol tcpip_linux_l04_symbols[] = {
    L04_SYMBOL(TCPIP_LINUX_L04_NETIF_RECEIVE_SKB, "netif_receive_skb", "netif_receive_skb",
               "net/core/dev.c", 5805U, "Enter the receive core with one sk_buff."),
    L04_SYMBOL(TCPIP_LINUX_L04_NETIF_RECEIVE_SKB_ONE_CORE, "__netif_receive_skb_one_core",
               "__netif_receive_skb_one_core", "net/core/dev.c", 5544U,
               "Run the protocol receive path for one packet."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_RCV, "ip_rcv", "ip_rcv", "net/ipv4/ip_input.c", 560U,
               "Validate an IPv4 packet before the pre-routing hook."),
    L04_SYMBOL(TCPIP_LINUX_L04_NF_INET_PRE_ROUTING, "nf_inet_pre_routing", "NF_INET_PRE_ROUTING",
               "net/ipv4/ip_input.c", 569U,
               "Run IPv4 Netfilter pre-routing before the ip_rcv_finish continuation."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_RCV_FINISH, "ip_rcv_finish", "ip_rcv_finish",
               "net/ipv4/ip_input.c", 435U,
               "Continue after pre-routing, route the skb, and dispatch dst_input."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF, "ip_route_input_noref",
               "ip_route_input_noref", "net/ipv4/route.c", 2487U,
               "Reuse or compute a route for an incoming packet."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW, "ip_route_input_slow", "ip_route_input_slow",
               "net/ipv4/route.c", 2223U, "Perform the uncached IPv4 input route lookup."),
    L04_SYMBOL(TCPIP_LINUX_L04_DST_INPUT, "dst_input", "dst_input", "include/net/dst.h", 466U,
               "Invoke the route destination input callback, usually ip_local_deliver for local IPv4."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_LOCAL_DELIVER, "ip_local_deliver", "ip_local_deliver",
               "net/ipv4/ip_input.c", 242U, "Reassemble if needed and enter local delivery hooks."),
    L04_SYMBOL(TCPIP_LINUX_L04_NF_INET_LOCAL_IN, "nf_inet_local_in", "NF_INET_LOCAL_IN",
               "net/ipv4/ip_input.c", 254U,
               "Run IPv4 Netfilter local-input before the ip_local_deliver_finish continuation."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_LOCAL_DELIVER_FINISH, "ip_local_deliver_finish",
               "ip_local_deliver_finish", "net/ipv4/ip_input.c", 227U,
               "Dispatch a locally delivered datagram to its IP protocol handler."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_V4_RCV, "tcp_v4_rcv", "tcp_v4_rcv", "net/ipv4/tcp_ipv4.c",
               1983U, "Receive and validate an IPv4 TCP segment."),
    L04_SYMBOL(TCPIP_LINUX_L04_INET_LOOKUP_SKB, "__inet_lookup_skb", "__inet_lookup_skb",
               "include/net/inet_hashtables.h", 492U,
               "Select an established or listening socket for an sk_buff."),
    L04_SYMBOL(TCPIP_LINUX_L04_INET_LOOKUP_ESTABLISHED, "__inet_lookup_established",
               "__inet_lookup_established", "include/net/inet_hashtables.h", 376U,
               "Search the established transport socket hash."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_RCV_ESTABLISHED, "tcp_rcv_established", "tcp_rcv_established",
               "net/ipv4/tcp_input.c", 5882U, "Process a segment on the TCP established fast or slow path."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_ACK, "tcp_ack", "tcp_ack", "net/ipv4/tcp_input.c", 3784U,
               "Apply an acknowledgment to TCP sender state."),
    L04_SYMBOL(TCPIP_LINUX_L04_SOCK_SENDMSG, "sock_sendmsg", "sock_sendmsg", "net/socket.c", 756U,
               "Enter protocol sendmsg through the socket layer."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_SENDMSG, "tcp_sendmsg", "tcp_sendmsg", "net/ipv4/tcp.c", 1335U,
               "Acquire the socket lock for a TCP send request."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_SENDMSG_LOCKED, "tcp_sendmsg_locked", "tcp_sendmsg_locked",
               "net/ipv4/tcp.c", 1037U, "Queue application bytes into TCP write buffers."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_WRITE_XMIT, "tcp_write_xmit", "tcp_write_xmit",
               "net/ipv4/tcp_output.c", 2670U, "Choose queued TCP segments eligible for transmission."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_TRANSMIT_SKB, "tcp_transmit_skb", "tcp_transmit_skb",
               "net/ipv4/tcp_output.c", 1430U,
               "Wrap __tcp_transmit_skb, which builds TCP headers and calls queue_xmit."),
    L04_SYMBOL(TCPIP_LINUX_L04_INET_QUEUE_XMIT, "inet_queue_xmit", "icsk_af_ops->queue_xmit",
               "net/ipv4/tcp_output.c", 1415U,
               "Address-family queue_xmit callback; IPv4 sockets use ip_queue_xmit."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW, "ip_route_output_flow", "ip_route_output_flow",
               "net/ipv4/route.c", 2869U, "Resolve an IPv4 output route for a flow."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_QUEUE_XMIT, "ip_queue_xmit", "ip_queue_xmit",
               "net/ipv4/ip_output.c", 545U, "IPv4 transport queueing callback."),
    L04_SYMBOL(TCPIP_LINUX_L04___IP_QUEUE_XMIT, "__ip_queue_xmit", "__ip_queue_xmit",
               "net/ipv4/ip_output.c", 454U, "Find or reuse a route, build the IPv4 header, and enter local output."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_LOCAL_OUT, "ip_local_out", "ip_local_out",
               "net/ipv4/ip_output.c", 121U, "Run local IPv4 output hooks before dst_output."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_OUTPUT, "ip_output", "ip_output", "net/ipv4/ip_output.c", 424U,
               "Run post-routing and choose the output device path."),
    L04_SYMBOL(TCPIP_LINUX_L04_IP_FINISH_OUTPUT, "ip_finish_output", "ip_finish_output",
               "net/ipv4/ip_output.c", 314U, "Apply final IPv4 output handling before neighbor output."),
    L04_SYMBOL(TCPIP_LINUX_L04___DEV_QUEUE_XMIT, "__dev_queue_xmit", "__dev_queue_xmit",
               "net/core/dev.c", 4277U, "Queue an sk_buff to device traffic control or a driver."),
    L04_SYMBOL(TCPIP_LINUX_L04_TCP_GET_INFO, "tcp_get_info", "tcp_get_info", "net/ipv4/tcp.c", 3699U,
               "Bridge internal TCP state to the TCP_INFO observation API.")};

static const tcpip_linux_l04_symbol_id tcpip_linux_l04_ingress[] = {
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

static const tcpip_linux_l04_symbol_id tcpip_linux_l04_egress[] = {
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

static const tcpip_linux_l04_edge tcpip_linux_l04_edges[] = {
    {TCPIP_LINUX_L04_NETIF_RECEIVE_SKB, TCPIP_LINUX_L04_NETIF_RECEIVE_SKB_ONE_CORE},
    {TCPIP_LINUX_L04_NETIF_RECEIVE_SKB_ONE_CORE, TCPIP_LINUX_L04_IP_RCV},
    {TCPIP_LINUX_L04_IP_RCV, TCPIP_LINUX_L04_NF_INET_PRE_ROUTING},
    {TCPIP_LINUX_L04_NF_INET_PRE_ROUTING, TCPIP_LINUX_L04_IP_RCV_FINISH},
    {TCPIP_LINUX_L04_IP_RCV_FINISH, TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF},
    {TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF, TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW},
    {TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW, TCPIP_LINUX_L04_DST_INPUT},
    {TCPIP_LINUX_L04_DST_INPUT, TCPIP_LINUX_L04_IP_LOCAL_DELIVER},
    {TCPIP_LINUX_L04_IP_LOCAL_DELIVER, TCPIP_LINUX_L04_NF_INET_LOCAL_IN},
    {TCPIP_LINUX_L04_NF_INET_LOCAL_IN, TCPIP_LINUX_L04_IP_LOCAL_DELIVER_FINISH},
    {TCPIP_LINUX_L04_IP_LOCAL_DELIVER_FINISH, TCPIP_LINUX_L04_TCP_V4_RCV},
    {TCPIP_LINUX_L04_TCP_V4_RCV, TCPIP_LINUX_L04_INET_LOOKUP_SKB},
    {TCPIP_LINUX_L04_INET_LOOKUP_SKB, TCPIP_LINUX_L04_INET_LOOKUP_ESTABLISHED},
    {TCPIP_LINUX_L04_INET_LOOKUP_ESTABLISHED, TCPIP_LINUX_L04_TCP_RCV_ESTABLISHED},
    {TCPIP_LINUX_L04_TCP_RCV_ESTABLISHED, TCPIP_LINUX_L04_TCP_ACK},
    {TCPIP_LINUX_L04_SOCK_SENDMSG, TCPIP_LINUX_L04_TCP_SENDMSG},
    {TCPIP_LINUX_L04_TCP_SENDMSG, TCPIP_LINUX_L04_TCP_SENDMSG_LOCKED},
    {TCPIP_LINUX_L04_TCP_SENDMSG_LOCKED, TCPIP_LINUX_L04_TCP_WRITE_XMIT},
    {TCPIP_LINUX_L04_TCP_WRITE_XMIT, TCPIP_LINUX_L04_TCP_TRANSMIT_SKB},
    {TCPIP_LINUX_L04_TCP_TRANSMIT_SKB, TCPIP_LINUX_L04_INET_QUEUE_XMIT},
    {TCPIP_LINUX_L04_INET_QUEUE_XMIT, TCPIP_LINUX_L04_IP_QUEUE_XMIT},
    {TCPIP_LINUX_L04_IP_QUEUE_XMIT, TCPIP_LINUX_L04___IP_QUEUE_XMIT},
    {TCPIP_LINUX_L04___IP_QUEUE_XMIT, TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW},
    {TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW, TCPIP_LINUX_L04_IP_LOCAL_OUT},
    {TCPIP_LINUX_L04___IP_QUEUE_XMIT, TCPIP_LINUX_L04_IP_LOCAL_OUT},
    {TCPIP_LINUX_L04_IP_LOCAL_OUT, TCPIP_LINUX_L04_IP_OUTPUT},
    {TCPIP_LINUX_L04_IP_OUTPUT, TCPIP_LINUX_L04_IP_FINISH_OUTPUT},
    {TCPIP_LINUX_L04_IP_FINISH_OUTPUT, TCPIP_LINUX_L04___DEV_QUEUE_XMIT}};

static int tcpip_linux_l04_id_in_range(tcpip_linux_l04_symbol_id id) {
  return id >= TCPIP_LINUX_L04_NETIF_RECEIVE_SKB && id <= TCPIP_LINUX_L04_TCP_GET_INFO;
}

tcpip_linux_l04_status tcpip_linux_l04_symbol_by_id(
    tcpip_linux_l04_symbol_id id,
    const tcpip_linux_l04_symbol **out_symbol) {
  if (out_symbol == NULL) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  *out_symbol = NULL;
  if (!tcpip_linux_l04_id_in_range(id)) {
    return TCPIP_LINUX_L04_NOT_FOUND;
  }
  *out_symbol = &tcpip_linux_l04_symbols[(size_t)id];
  return TCPIP_LINUX_L04_OK;
}

tcpip_linux_l04_status tcpip_linux_l04_validate_path(
    const tcpip_linux_l04_symbol_id *path,
    size_t path_length) {
  size_t index;
  size_t edge_index;

  if (path == NULL || path_length == 0U) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  for (index = 0U; index < path_length; index += 1U) {
    if (!tcpip_linux_l04_id_in_range(path[index])) {
      return TCPIP_LINUX_L04_NOT_FOUND;
    }
  }
  for (index = 1U; index < path_length; index += 1U) {
    int found = 0;
    for (edge_index = 0U; edge_index < sizeof(tcpip_linux_l04_edges) / sizeof(tcpip_linux_l04_edges[0]);
         edge_index += 1U) {
      if (tcpip_linux_l04_edges[edge_index].from == path[index - 1U] &&
          tcpip_linux_l04_edges[edge_index].to == path[index]) {
        found = 1;
        break;
      }
    }
    if (found == 0) {
      return TCPIP_LINUX_L04_INVALID_PATH;
    }
  }
  return TCPIP_LINUX_L04_OK;
}

tcpip_linux_l04_status tcpip_linux_l04_format_source_url(
    tcpip_linux_l04_symbol_id id,
    char *output,
    size_t output_capacity,
    size_t *written) {
  const tcpip_linux_l04_symbol *symbol = NULL;
  int required;

  if (written != NULL) {
    *written = 0U;
  }
  if (output != NULL && output_capacity > 0U) {
    output[0] = '\0';
  }
  if (written == NULL) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  if (output == NULL && output_capacity != 0U) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  if (tcpip_linux_l04_symbol_by_id(id, &symbol) != TCPIP_LINUX_L04_OK) {
    return TCPIP_LINUX_L04_NOT_FOUND;
  }
  required = snprintf(
      NULL,
      0U,
      "https://github.com/torvalds/linux/blob/v6.6/%s#L%u",
      symbol->source_path,
      symbol->source_line);
  if (required < 0) {
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  *written = (size_t)required;
  if (output_capacity == 0U || (size_t)required >= output_capacity) {
    return TCPIP_LINUX_L04_CAPACITY;
  }
  required = snprintf(
      output,
      output_capacity,
      "https://github.com/torvalds/linux/blob/v6.6/%s#L%u",
      symbol->source_path,
      symbol->source_line);
  if (required < 0) {
    output[0] = '\0';
    *written = 0U;
    return TCPIP_LINUX_L04_INVALID_ARGUMENT;
  }
  return TCPIP_LINUX_L04_OK;
}

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_ingress_path(size_t *count) {
  if (count == NULL) {
    return NULL;
  }
  *count = sizeof(tcpip_linux_l04_ingress) / sizeof(tcpip_linux_l04_ingress[0]);
  return tcpip_linux_l04_ingress;
}

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_egress_path(size_t *count) {
  if (count == NULL) {
    return NULL;
  }
  *count = sizeof(tcpip_linux_l04_egress) / sizeof(tcpip_linux_l04_egress[0]);
  return tcpip_linux_l04_egress;
}
