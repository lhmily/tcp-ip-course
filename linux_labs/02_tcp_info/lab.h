#ifndef TCPIP_LINUX_L02_LAB_H
#define TCPIP_LINUX_L02_LAB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tcpip_linux_l02_status {
  TCPIP_LINUX_L02_OK = 0,
  TCPIP_LINUX_L02_INVALID_ARGUMENT = 1,
  TCPIP_LINUX_L02_UNSUPPORTED = 2,
  TCPIP_LINUX_L02_SYSTEM = 3,
  TCPIP_LINUX_L02_TODO = 4
} tcpip_linux_l02_status;

typedef enum tcpip_linux_l02_presence {
  TCPIP_LINUX_L02_HAS_STATE = UINT32_C(1) << 0,
  TCPIP_LINUX_L02_HAS_CA_STATE = UINT32_C(1) << 1,
  TCPIP_LINUX_L02_HAS_RETRANSMITS = UINT32_C(1) << 2,
  TCPIP_LINUX_L02_HAS_SND_MSS = UINT32_C(1) << 3,
  TCPIP_LINUX_L02_HAS_RCV_MSS = UINT32_C(1) << 4,
  TCPIP_LINUX_L02_HAS_UNACKED = UINT32_C(1) << 5,
  TCPIP_LINUX_L02_HAS_LOST = UINT32_C(1) << 6,
  TCPIP_LINUX_L02_HAS_RTT_US = UINT32_C(1) << 7,
  TCPIP_LINUX_L02_HAS_RTTVAR_US = UINT32_C(1) << 8,
  TCPIP_LINUX_L02_HAS_TOTAL_RETRANS = UINT32_C(1) << 9
} tcpip_linux_l02_presence;

typedef struct tcpip_linux_l02_snapshot {
  uint32_t present;
  uint8_t state;
  uint8_t ca_state;
  uint8_t retransmits;
  uint32_t snd_mss;
  uint32_t rcv_mss;
  uint32_t unacked;
  uint32_t lost;
  uint32_t rtt_us;
  uint32_t rttvar_us;
  uint32_t total_retrans;
} tcpip_linux_l02_snapshot;

typedef enum tcpip_linux_l02_model_state {
  TCPIP_LINUX_L02_MODEL_CLOSED = 0,
  TCPIP_LINUX_L02_MODEL_LISTEN = 1,
  TCPIP_LINUX_L02_MODEL_SYN_SENT = 2,
  TCPIP_LINUX_L02_MODEL_SYN_RECEIVED = 3,
  TCPIP_LINUX_L02_MODEL_ESTABLISHED = 4,
  TCPIP_LINUX_L02_MODEL_FIN_WAIT_1 = 5,
  TCPIP_LINUX_L02_MODEL_FIN_WAIT_2 = 6,
  TCPIP_LINUX_L02_MODEL_CLOSE_WAIT = 7,
  TCPIP_LINUX_L02_MODEL_LAST_ACK = 8,
  TCPIP_LINUX_L02_MODEL_TIME_WAIT = 9
} tcpip_linux_l02_model_state;

typedef enum tcpip_linux_l02_violation_bits {
  TCPIP_LINUX_L02_VIOLATION_MISSING_STATE = UINT32_C(1) << 0,
  TCPIP_LINUX_L02_VIOLATION_UNKNOWN_STATE = UINT32_C(1) << 1,
  TCPIP_LINUX_L02_VIOLATION_ZERO_SND_MSS = UINT32_C(1) << 2,
  TCPIP_LINUX_L02_VIOLATION_ZERO_RCV_MSS = UINT32_C(1) << 3,
  TCPIP_LINUX_L02_VIOLATION_ZERO_RTT = UINT32_C(1) << 4,
  TCPIP_LINUX_L02_VIOLATION_LOST_WITHOUT_RETRANS = UINT32_C(1) << 5,
  TCPIP_LINUX_L02_VIOLATION_UNKNOWN_PRESENCE = UINT32_C(1) << 6
} tcpip_linux_l02_violation_bits;

typedef struct tcpip_linux_l02_violations {
  uint32_t bits;
  uint32_t count;
} tcpip_linux_l02_violations;

tcpip_linux_l02_status tcpip_linux_l02_capture(
    int fd,
    tcpip_linux_l02_snapshot *snapshot);

tcpip_linux_l02_status tcpip_linux_l02_map_state(
    uint8_t linux_state,
    tcpip_linux_l02_model_state *model_state);

tcpip_linux_l02_status tcpip_linux_l02_check_invariants(
    const tcpip_linux_l02_snapshot *snapshot,
    tcpip_linux_l02_violations *violations);

#ifdef __cplusplus
}
#endif

#endif
