#include "lesson.h"

#include <string.h>

tcpip_l12_status tcpip_l12_diagnose_frame(
    const uint8_t *data, size_t len, tcpip_l12_report *report) {
  if (report == NULL || (data == NULL && len != 0U)) {
    return TCPIP_L12_INVALID_ARGUMENT;
  }

  memset(report, 0, sizeof(*report));
  report->frame_length = len;
  return TCPIP_L12_TODO;
}

tcpip_l12_status tcpip_l12_format_report(
    const tcpip_l12_report *report, char *out, size_t cap, size_t *written) {
  if (report == NULL || written == NULL || (out == NULL && cap != 0U)) {
    return TCPIP_L12_INVALID_ARGUMENT;
  }

  *written = 0U;
  if (cap != 0U) {
    out[0] = '\0';
  }
  return TCPIP_L12_TODO;
}
