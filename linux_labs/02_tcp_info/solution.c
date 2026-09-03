#include "lab.h"

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>

#ifndef TCP_NEW_SYN_RECV
#define TCP_NEW_SYN_RECV 12
#endif

#define TCPIP_LINUX_L02_KNOWN_PRESENCE \
  (TCPIP_LINUX_L02_HAS_STATE | TCPIP_LINUX_L02_HAS_CA_STATE | \
   TCPIP_LINUX_L02_HAS_RETRANSMITS | TCPIP_LINUX_L02_HAS_SND_MSS | \
   TCPIP_LINUX_L02_HAS_RCV_MSS | TCPIP_LINUX_L02_HAS_UNACKED | \
   TCPIP_LINUX_L02_HAS_LOST | TCPIP_LINUX_L02_HAS_RTT_US | \
   TCPIP_LINUX_L02_HAS_RTTVAR_US | TCPIP_LINUX_L02_HAS_TOTAL_RETRANS)

#define TCPIP_LINUX_L02_FIELD_AVAILABLE(length, type, member) \
  ((size_t)(length) >= offsetof(type, member) + sizeof(((type *)0)->member))

static void tcpip_linux_l02_clear_snapshot(tcpip_linux_l02_snapshot *snapshot) {
  if (snapshot != NULL) {
    memset(snapshot, 0, sizeof(*snapshot));
  }
}

static void tcpip_linux_l02_clear_violations(tcpip_linux_l02_violations *violations) {
  if (violations != NULL) {
    memset(violations, 0, sizeof(*violations));
  }
}

static void tcpip_linux_l02_add_violation(
    tcpip_linux_l02_violations *violations,
    uint32_t bit) {
  if ((violations->bits & bit) == 0U) {
    violations->count += 1U;
  }
  violations->bits |= bit;
}

tcpip_linux_l02_status tcpip_linux_l02_capture(
    int fd,
    tcpip_linux_l02_snapshot *snapshot) {
  struct tcp_info info;
  socklen_t length = (socklen_t)sizeof(info);

  tcpip_linux_l02_clear_snapshot(snapshot);
  if (fd < 0 || snapshot == NULL) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }

  memset(&info, 0, sizeof(info));
  if (getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &length) < 0) {
    if (errno == ENOPROTOOPT || errno == EOPNOTSUPP || errno == ENOTSOCK ||
        errno == EINVAL) {
      return TCPIP_LINUX_L02_UNSUPPORTED;
    }
    return TCPIP_LINUX_L02_SYSTEM;
  }

  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_state)) {
    snapshot->state = info.tcpi_state;
    snapshot->present |= TCPIP_LINUX_L02_HAS_STATE;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_ca_state)) {
    snapshot->ca_state = info.tcpi_ca_state;
    snapshot->present |= TCPIP_LINUX_L02_HAS_CA_STATE;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_retransmits)) {
    snapshot->retransmits = info.tcpi_retransmits;
    snapshot->present |= TCPIP_LINUX_L02_HAS_RETRANSMITS;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_snd_mss)) {
    snapshot->snd_mss = info.tcpi_snd_mss;
    snapshot->present |= TCPIP_LINUX_L02_HAS_SND_MSS;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_rcv_mss)) {
    snapshot->rcv_mss = info.tcpi_rcv_mss;
    snapshot->present |= TCPIP_LINUX_L02_HAS_RCV_MSS;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_unacked)) {
    snapshot->unacked = info.tcpi_unacked;
    snapshot->present |= TCPIP_LINUX_L02_HAS_UNACKED;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_lost)) {
    snapshot->lost = info.tcpi_lost;
    snapshot->present |= TCPIP_LINUX_L02_HAS_LOST;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_rtt)) {
    snapshot->rtt_us = info.tcpi_rtt;
    snapshot->present |= TCPIP_LINUX_L02_HAS_RTT_US;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_rttvar)) {
    snapshot->rttvar_us = info.tcpi_rttvar;
    snapshot->present |= TCPIP_LINUX_L02_HAS_RTTVAR_US;
  }
  if (TCPIP_LINUX_L02_FIELD_AVAILABLE(length, struct tcp_info, tcpi_total_retrans)) {
    snapshot->total_retrans = info.tcpi_total_retrans;
    snapshot->present |= TCPIP_LINUX_L02_HAS_TOTAL_RETRANS;
  }

  return TCPIP_LINUX_L02_OK;
}

tcpip_linux_l02_status tcpip_linux_l02_map_state(
    uint8_t linux_state,
    tcpip_linux_l02_model_state *model_state) {
  if (model_state == NULL) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }
  *model_state = TCPIP_LINUX_L02_MODEL_CLOSED;
  switch (linux_state) {
    case TCP_CLOSE:
      *model_state = TCPIP_LINUX_L02_MODEL_CLOSED;
      return TCPIP_LINUX_L02_OK;
    case TCP_LISTEN:
      *model_state = TCPIP_LINUX_L02_MODEL_LISTEN;
      return TCPIP_LINUX_L02_OK;
    case TCP_SYN_SENT:
      *model_state = TCPIP_LINUX_L02_MODEL_SYN_SENT;
      return TCPIP_LINUX_L02_OK;
    case TCP_SYN_RECV:
      *model_state = TCPIP_LINUX_L02_MODEL_SYN_RECEIVED;
      return TCPIP_LINUX_L02_OK;
    case TCP_ESTABLISHED:
      *model_state = TCPIP_LINUX_L02_MODEL_ESTABLISHED;
      return TCPIP_LINUX_L02_OK;
    case TCP_FIN_WAIT1:
      *model_state = TCPIP_LINUX_L02_MODEL_FIN_WAIT_1;
      return TCPIP_LINUX_L02_OK;
    case TCP_FIN_WAIT2:
      *model_state = TCPIP_LINUX_L02_MODEL_FIN_WAIT_2;
      return TCPIP_LINUX_L02_OK;
    case TCP_CLOSE_WAIT:
      *model_state = TCPIP_LINUX_L02_MODEL_CLOSE_WAIT;
      return TCPIP_LINUX_L02_OK;
    case TCP_LAST_ACK:
      *model_state = TCPIP_LINUX_L02_MODEL_LAST_ACK;
      return TCPIP_LINUX_L02_OK;
    case TCP_TIME_WAIT:
      *model_state = TCPIP_LINUX_L02_MODEL_TIME_WAIT;
      return TCPIP_LINUX_L02_OK;
    case TCP_CLOSING:
      *model_state = TCPIP_LINUX_L02_MODEL_FIN_WAIT_1;
      return TCPIP_LINUX_L02_OK;
    case TCP_NEW_SYN_RECV:
      *model_state = TCPIP_LINUX_L02_MODEL_SYN_RECEIVED;
      return TCPIP_LINUX_L02_OK;
    default:
      return TCPIP_LINUX_L02_UNSUPPORTED;
  }
}

tcpip_linux_l02_status tcpip_linux_l02_check_invariants(
    const tcpip_linux_l02_snapshot *snapshot,
    tcpip_linux_l02_violations *violations) {
  tcpip_linux_l02_model_state model_state;

  tcpip_linux_l02_clear_violations(violations);
  if (snapshot == NULL || violations == NULL) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }
  if ((snapshot->present & ~(uint32_t)TCPIP_LINUX_L02_KNOWN_PRESENCE) != 0U) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_UNKNOWN_PRESENCE);
  }
  if ((snapshot->present & TCPIP_LINUX_L02_HAS_STATE) == 0U) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_MISSING_STATE);
  } else if (tcpip_linux_l02_map_state(snapshot->state, &model_state) !=
             TCPIP_LINUX_L02_OK) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_UNKNOWN_STATE);
  }
  if ((snapshot->present & TCPIP_LINUX_L02_HAS_SND_MSS) != 0U &&
      snapshot->snd_mss == 0U) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_ZERO_SND_MSS);
  }
  if ((snapshot->present & TCPIP_LINUX_L02_HAS_RCV_MSS) != 0U &&
      snapshot->rcv_mss == 0U) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_ZERO_RCV_MSS);
  }
  if ((snapshot->present & TCPIP_LINUX_L02_HAS_RTT_US) != 0U &&
      snapshot->rtt_us == 0U) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_ZERO_RTT);
  }
  if ((snapshot->present & TCPIP_LINUX_L02_HAS_LOST) != 0U &&
      snapshot->lost != 0U &&
      ((snapshot->present & TCPIP_LINUX_L02_HAS_TOTAL_RETRANS) == 0U ||
       snapshot->total_retrans == 0U) &&
      ((snapshot->present & TCPIP_LINUX_L02_HAS_RETRANSMITS) == 0U ||
       snapshot->retransmits == 0U)) {
    tcpip_linux_l02_add_violation(
        violations, TCPIP_LINUX_L02_VIOLATION_LOST_WITHOUT_RETRANS);
  }

  return TCPIP_LINUX_L02_OK;
}
