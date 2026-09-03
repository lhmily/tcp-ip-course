#ifndef TCPIP_LINUX_L04_LAB_H
#define TCPIP_LINUX_L04_LAB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_LINUX_L04_VERSION "v6.6"
#define TCPIP_LINUX_L04_SYMBOL_COUNT 26U
#define TCPIP_LINUX_L04_INGRESS_COUNT 13U
#define TCPIP_LINUX_L04_EGRESS_COUNT 12U

typedef enum tcpip_linux_l04_status {
  TCPIP_LINUX_L04_OK = 0,
  TCPIP_LINUX_L04_INVALID_ARGUMENT,
  TCPIP_LINUX_L04_NOT_FOUND,
  TCPIP_LINUX_L04_INVALID_PATH,
  TCPIP_LINUX_L04_CAPACITY,
  TCPIP_LINUX_L04_TODO
} tcpip_linux_l04_status;

typedef enum tcpip_linux_l04_symbol_id {
  TCPIP_LINUX_L04_NETIF_RECEIVE_SKB = 0,
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
  TCPIP_LINUX_L04_TCP_ACK,
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
  TCPIP_LINUX_L04___DEV_QUEUE_XMIT,
  TCPIP_LINUX_L04_TCP_GET_INFO
} tcpip_linux_l04_symbol_id;

typedef struct tcpip_linux_l04_symbol {
  tcpip_linux_l04_symbol_id id;
  const char *key;
  const char *name;
  const char *source_path;
  unsigned source_line;
  const char *description;
} tcpip_linux_l04_symbol;

/* Lookup is total over the authored index and returns NOT_FOUND for unknown IDs. */
tcpip_linux_l04_status tcpip_linux_l04_symbol_by_id(
    tcpip_linux_l04_symbol_id id,
    const tcpip_linux_l04_symbol **out_symbol);

/* Validate that every neighboring pair is an authored directed call edge. */
tcpip_linux_l04_status tcpip_linux_l04_validate_path(
    const tcpip_linux_l04_symbol_id *path,
    size_t path_length);

/* Format an immutable HTTPS link pinned to the Linux v6.6 source tree. */
tcpip_linux_l04_status tcpip_linux_l04_format_source_url(
    tcpip_linux_l04_symbol_id id,
    char *output,
    size_t output_capacity,
    size_t *written);

const tcpip_linux_l04_symbol_id *tcpip_linux_l04_ingress_path(size_t *count);
const tcpip_linux_l04_symbol_id *tcpip_linux_l04_egress_path(size_t *count);

#ifdef __cplusplus
}
#endif

#endif
