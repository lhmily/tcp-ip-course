#include "lesson.h"
#include "tcpip/test.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct transition_case {
  tcpip_l07_tcp_state from;
  tcpip_l07_tcp_event event;
  tcpip_l07_tcp_state to;
} transition_case;

static void test_transitions(tcpip_test_context *test) {
  static const transition_case cases[] = {
      {TCPIP_L07_TCP_STATE_CLOSED, TCPIP_L07_TCP_EVENT_PASSIVE_OPEN, TCPIP_L07_TCP_STATE_LISTEN},
      {TCPIP_L07_TCP_STATE_CLOSED, TCPIP_L07_TCP_EVENT_ACTIVE_OPEN, TCPIP_L07_TCP_STATE_SYN_SENT},
      {TCPIP_L07_TCP_STATE_LISTEN, TCPIP_L07_TCP_EVENT_RECEIVE_SYN, TCPIP_L07_TCP_STATE_SYN_RECEIVED},
      {TCPIP_L07_TCP_STATE_LISTEN, TCPIP_L07_TCP_EVENT_APP_CLOSE, TCPIP_L07_TCP_STATE_CLOSED},
      {TCPIP_L07_TCP_STATE_SYN_SENT, TCPIP_L07_TCP_EVENT_RECEIVE_SYN, TCPIP_L07_TCP_STATE_SYN_RECEIVED},
      {TCPIP_L07_TCP_STATE_SYN_SENT, TCPIP_L07_TCP_EVENT_RECEIVE_SYN_ACK, TCPIP_L07_TCP_STATE_ESTABLISHED},
      {TCPIP_L07_TCP_STATE_SYN_RECEIVED, TCPIP_L07_TCP_EVENT_RECEIVE_ACK, TCPIP_L07_TCP_STATE_ESTABLISHED},
      {TCPIP_L07_TCP_STATE_ESTABLISHED, TCPIP_L07_TCP_EVENT_APP_CLOSE, TCPIP_L07_TCP_STATE_FIN_WAIT_1},
      {TCPIP_L07_TCP_STATE_ESTABLISHED, TCPIP_L07_TCP_EVENT_RECEIVE_FIN, TCPIP_L07_TCP_STATE_CLOSE_WAIT},
      {TCPIP_L07_TCP_STATE_FIN_WAIT_1, TCPIP_L07_TCP_EVENT_RECEIVE_ACK, TCPIP_L07_TCP_STATE_FIN_WAIT_2},
      {TCPIP_L07_TCP_STATE_FIN_WAIT_1, TCPIP_L07_TCP_EVENT_RECEIVE_FIN_ACK, TCPIP_L07_TCP_STATE_TIME_WAIT},
      {TCPIP_L07_TCP_STATE_FIN_WAIT_2, TCPIP_L07_TCP_EVENT_RECEIVE_FIN, TCPIP_L07_TCP_STATE_TIME_WAIT},
      {TCPIP_L07_TCP_STATE_CLOSE_WAIT, TCPIP_L07_TCP_EVENT_APP_CLOSE, TCPIP_L07_TCP_STATE_LAST_ACK},
      {TCPIP_L07_TCP_STATE_LAST_ACK, TCPIP_L07_TCP_EVENT_RECEIVE_ACK, TCPIP_L07_TCP_STATE_CLOSED},
      {TCPIP_L07_TCP_STATE_TIME_WAIT, TCPIP_L07_TCP_EVENT_TIMEOUT, TCPIP_L07_TCP_STATE_CLOSED},
  };

  for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); index += 1U) {
    tcpip_l07_tcp_state actual = TCPIP_L07_TCP_STATE_TIME_WAIT;
    const tcpip_l07_status status =
        tcpip_l07_transition(cases[index].from, cases[index].event, &actual);
    TCPIP_EXPECT_U32(test, status, TCPIP_L07_OK);
    if (status == TCPIP_L07_OK) {
      TCPIP_EXPECT_U32(test, actual, cases[index].to);
    }
  }

  tcpip_l07_tcp_state output = TCPIP_L07_TCP_STATE_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          TCPIP_L07_TCP_STATE_CLOSED, TCPIP_L07_TCP_EVENT_RECEIVE_FIN, &output),
      TCPIP_L07_MALFORMED);
  TCPIP_EXPECT_U32(test, output, TCPIP_L07_TCP_STATE_CLOSED);

  output = TCPIP_L07_TCP_STATE_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          (tcpip_l07_tcp_state)99, TCPIP_L07_TCP_EVENT_ACTIVE_OPEN, &output),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(test, output, TCPIP_L07_TCP_STATE_CLOSED);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          TCPIP_L07_TCP_STATE_CLOSED, TCPIP_L07_TCP_EVENT_ACTIVE_OPEN, NULL),
      TCPIP_L07_INVALID_ARGUMENT);
}

static void test_reassembly(tcpip_test_context *test) {
  uint8_t data[8] = {0U};
  uint8_t present[8];
  memset(present, 0xff, sizeof(present));
  tcpip_l07_reassembly reassembly;

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, UINT32_C(0xfffffffd), data, present, sizeof(data)),
      TCPIP_L07_OK);
  if (reassembly.capacity != sizeof(data)) {
    return;
  }
  static const uint8_t empty_present[8] = {0U};
  TCPIP_EXPECT_BYTES(test, present, sizeof(present), empty_present, sizeof(empty_present));

  size_t accepted = 99U;
  static const uint8_t def[] = {'D', 'E', 'F'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(&reassembly, UINT32_C(0), def, sizeof(def), &accepted),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, accepted, 3U);

  static const uint8_t absent_then_conflict[] = {'C', 'X'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(
          &reassembly, UINT32_C(0xffffffff), absent_then_conflict,
          sizeof(absent_then_conflict), &accepted),
      TCPIP_L07_MALFORMED);
  TCPIP_EXPECT_SIZE(test, accepted, 0U);
  TCPIP_EXPECT_U32(test, present[2], 0U);
  TCPIP_EXPECT_U32(test, data[2], 0U);

  uint8_t output[8] = {0U};
  size_t produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, output, sizeof(output), &produced),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, produced, 0U);

  static const uint8_t abc[] = {'A', 'B', 'C'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(
          &reassembly, UINT32_C(0xfffffffd), abc, sizeof(abc), &accepted),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, accepted, 3U);

  static const uint8_t bcd[] = {'B', 'C', 'D'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(
          &reassembly, UINT32_C(0xfffffffe), bcd, sizeof(bcd), &accepted),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, accepted, 0U);

  static const uint8_t conflict[] = {'B', 'X', 'D'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(
          &reassembly, UINT32_C(0xfffffffe), conflict, sizeof(conflict), &accepted),
      TCPIP_L07_MALFORMED);
  TCPIP_EXPECT_SIZE(test, accepted, 0U);
  static const uint8_t abcdef[] = {'A', 'B', 'C', 'D', 'E', 'F'};
  TCPIP_EXPECT_BYTES(test, data, sizeof(abcdef), abcdef, sizeof(abcdef));

  memset(output, 0xa5, sizeof(output));
  produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, output, 5U, &produced),
      TCPIP_L07_TRUNCATED);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
  TCPIP_EXPECT_U32(test, output[0], UINT8_C(0xa5));
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, output, 6U, &produced),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, produced, 6U);
  TCPIP_EXPECT_BYTES(test, output, produced, abcdef, sizeof(abcdef));

  static const uint8_t gh[] = {'G', 'H'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(&reassembly, UINT32_C(3), gh, sizeof(gh), &accepted),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, accepted, 2U);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, output, sizeof(output), &produced),
      TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, produced, 2U);
  TCPIP_EXPECT_BYTES(test, output, produced, gh, sizeof(gh));

  static const uint8_t overflow[] = {'H', 'Z'};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(&reassembly, UINT32_C(4), overflow, sizeof(overflow), &accepted),
      TCPIP_L07_CAPACITY);
  TCPIP_EXPECT_SIZE(test, accepted, 0U);
  TCPIP_EXPECT_U32(test, data[7], (uint8_t)'H');

  accepted = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(&reassembly, UINT32_C(5), NULL, 1U, &accepted),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, accepted, 0U);

  produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, NULL, 1U, &produced),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
}

static void test_initialization_failures(tcpip_test_context *test) {
  tcpip_l07_reassembly reassembly = {
      UINT32_MAX, (uint8_t *)(uintptr_t)1U, (uint8_t *)(uintptr_t)1U, 1U, 1U};
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, 7U, NULL, NULL, 1U),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(test, reassembly.initial_seq, 0U);
  TCPIP_EXPECT_TRUE(test, reassembly.data == NULL);
  TCPIP_EXPECT_TRUE(test, reassembly.present == NULL);
  TCPIP_EXPECT_SIZE(test, reassembly.capacity, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);

  TCPIP_EXPECT_U32(
      test, tcpip_l07_reassembly_init(&reassembly, 7U, NULL, NULL, 0U), TCPIP_L07_OK);
  size_t accepted = 99U;
  TCPIP_EXPECT_U32(
      test, tcpip_l07_reassembly_push(&reassembly, 123U, NULL, 0U, &accepted), TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, accepted, 0U);
  size_t produced = 99U;
  TCPIP_EXPECT_U32(
      test, tcpip_l07_reassembly_read(&reassembly, NULL, 0U, &produced), TCPIP_L07_OK);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "lesson 07 TCP state and reliability");
  test_transitions(&test);
  test_reassembly(&test);
  test_initialization_failures(&test);
  return tcpip_test_finish(&test);
}
