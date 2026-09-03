#ifndef TCPIP_LINUX_L03_LAB_H
#define TCPIP_LINUX_L03_LAB_H

#include <stddef.h>
#include <stdint.h>

#include "../../lessons/07_tcp_state_reliability/lesson.h"
#include "../../lessons/11_routing_nat/lesson.h"
#include "../../lessons/12_diagnostics_integration/lesson.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TCPIP_LINUX_L03_MAX_FAULT_ACTIONS 32U
#define TCPIP_LINUX_L03_MAX_EVENTS 32U
#define TCPIP_LINUX_L03_MAX_PAYLOAD 1024U
#define TCPIP_LINUX_L03_MAX_FRAME 1110U

typedef enum tcpip_linux_l03_status {
  TCPIP_LINUX_L03_OK = 0,
  TCPIP_LINUX_L03_INVALID_ARGUMENT,
  TCPIP_LINUX_L03_MALFORMED,
  TCPIP_LINUX_L03_CAPACITY,
  TCPIP_LINUX_L03_ROUTE_NOT_FOUND,
  TCPIP_LINUX_L03_RETRY_EXHAUSTED,
  TCPIP_LINUX_L03_SYSTEM_ERROR,
  TCPIP_LINUX_L03_TODO
} tcpip_linux_l03_status;

typedef enum tcpip_linux_l03_fault_action {
  TCPIP_LINUX_L03_DELIVER = 0,
  TCPIP_LINUX_L03_DROP,
  TCPIP_LINUX_L03_DELAY,
  TCPIP_LINUX_L03_REORDER
} tcpip_linux_l03_fault_action;

typedef struct tcpip_linux_l03_fault {
  tcpip_linux_l03_fault_action action;
  uint64_t delay_ms;
} tcpip_linux_l03_fault;

typedef struct tcpip_linux_l03_scenario {
  const uint8_t *payload;
  size_t payload_length;
  uint8_t *output;
  size_t output_capacity;

  const tcpip_l11_route *routes;
  size_t route_count;
  uint8_t source_ipv4[4];
  uint8_t destination_ipv4[4];
  uint16_t source_port;
  uint16_t destination_port;

  uint32_t sender_initial_seq;
  uint32_t receiver_initial_seq;
  uint64_t initial_rto_ms;
  size_t max_attempts;

  const tcpip_linux_l03_fault *faults;
  size_t fault_count;
} tcpip_linux_l03_scenario;

typedef struct tcpip_linux_l03_result {
  size_t route_index;
  uint64_t virtual_elapsed_ms;
  size_t attempts;
  size_t retransmits;
  tcpip_l07_tcp_state final_state;
  size_t reassembled_length;
  tcpip_l12_report final_frame_report;
  char final_frame_diagnostic[512];
  size_t final_frame_diagnostic_length;
} tcpip_linux_l03_result;

/* Run one bounded, deterministic userspace TCP-over-IPv4 simulation. */
tcpip_linux_l03_status tcpip_linux_l03_run(
    const tcpip_linux_l03_scenario *scenario,
    tcpip_linux_l03_result *result);

#ifdef __cplusplus
}
#endif

#endif
