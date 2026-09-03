#include "lab.h"

#include <string.h>

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

tcpip_linux_l02_status tcpip_linux_l02_capture(
    int fd,
    tcpip_linux_l02_snapshot *snapshot) {
  tcpip_linux_l02_clear_snapshot(snapshot);
  if (fd < 0 || snapshot == NULL) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }
  return TCPIP_LINUX_L02_TODO;
}

tcpip_linux_l02_status tcpip_linux_l02_map_state(
    uint8_t linux_state,
    tcpip_linux_l02_model_state *model_state) {
  if (model_state == NULL) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }
  *model_state = TCPIP_LINUX_L02_MODEL_CLOSED;
  if (linux_state == 0U) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }
  return TCPIP_LINUX_L02_TODO;
}

tcpip_linux_l02_status tcpip_linux_l02_check_invariants(
    const tcpip_linux_l02_snapshot *snapshot,
    tcpip_linux_l02_violations *violations) {
  tcpip_linux_l02_clear_violations(violations);
  if (snapshot == NULL || violations == NULL) {
    return TCPIP_LINUX_L02_INVALID_ARGUMENT;
  }
  return TCPIP_LINUX_L02_TODO;
}
