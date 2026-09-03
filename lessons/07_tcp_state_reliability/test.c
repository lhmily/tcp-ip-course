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

  for (int state = TCPIP_L07_TCP_STATE_CLOSED;
       state <= TCPIP_L07_TCP_STATE_TIME_WAIT;
       state += 1) {
    for (int event = TCPIP_L07_TCP_EVENT_PASSIVE_OPEN;
         event <= TCPIP_L07_TCP_EVENT_TIMEOUT;
         event += 1) {
      int allowed = 0;
      for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); index += 1U) {
        if (cases[index].from == (tcpip_l07_tcp_state)state &&
            cases[index].event == (tcpip_l07_tcp_event)event) {
          allowed = 1;
          break;
        }
      }
      if (!allowed) {
        tcpip_l07_tcp_state output = TCPIP_L07_TCP_STATE_TIME_WAIT;
        TCPIP_EXPECT_U32(
            test,
            tcpip_l07_transition(
                (tcpip_l07_tcp_state)state, (tcpip_l07_tcp_event)event, &output),
            TCPIP_L07_MALFORMED);
        TCPIP_EXPECT_U32(test, output, TCPIP_L07_TCP_STATE_CLOSED);
      }
    }
  }

  tcpip_l07_tcp_state output = TCPIP_L07_TCP_STATE_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          (tcpip_l07_tcp_state)-1, TCPIP_L07_TCP_EVENT_ACTIVE_OPEN, &output),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(test, output, TCPIP_L07_TCP_STATE_CLOSED);

  output = TCPIP_L07_TCP_STATE_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          TCPIP_L07_TCP_STATE_CLOSED, (tcpip_l07_tcp_event)-1, &output),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(test, output, TCPIP_L07_TCP_STATE_CLOSED);

  output = TCPIP_L07_TCP_STATE_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          (tcpip_l07_tcp_state)99, TCPIP_L07_TCP_EVENT_ACTIVE_OPEN, &output),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(test, output, TCPIP_L07_TCP_STATE_CLOSED);

  output = TCPIP_L07_TCP_STATE_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_transition(
          TCPIP_L07_TCP_STATE_CLOSED, (tcpip_l07_tcp_event)99, &output),
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

static void expect_empty_reassembly(
    tcpip_test_context *test,
    const tcpip_l07_reassembly *reassembly) {
  TCPIP_EXPECT_U32(test, reassembly->initial_seq, 0U);
  TCPIP_EXPECT_TRUE(test, reassembly->data == NULL);
  TCPIP_EXPECT_TRUE(test, reassembly->present == NULL);
  TCPIP_EXPECT_SIZE(test, reassembly->capacity, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly->read_offset, 0U);
}

static void test_initialization_failures(tcpip_test_context *test) {
  tcpip_l07_reassembly reassembly = {
      UINT32_MAX, (uint8_t *)(uintptr_t)1U, (uint8_t *)(uintptr_t)1U, 1U, 1U};
  uint8_t storage[16];
  uint8_t expected[16];

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, 7U, NULL, NULL, 1U),
      TCPIP_L07_INVALID_ARGUMENT);
  expect_empty_reassembly(test, &reassembly);

  memset(storage, 0xa5, sizeof(storage));
  memcpy(expected, storage, sizeof(expected));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, 7U, storage, storage, 8U),
      TCPIP_L07_INVALID_ARGUMENT);
  expect_empty_reassembly(test, &reassembly);
  TCPIP_EXPECT_BYTES(test, storage, sizeof(storage), expected, sizeof(expected));

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, 7U, storage, storage + 4U, 8U),
      TCPIP_L07_INVALID_ARGUMENT);
  expect_empty_reassembly(test, &reassembly);
  TCPIP_EXPECT_BYTES(test, storage, sizeof(storage), expected, sizeof(expected));

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, 7U, storage + 4U, storage, 8U),
      TCPIP_L07_INVALID_ARGUMENT);
  expect_empty_reassembly(test, &reassembly);
  TCPIP_EXPECT_BYTES(test, storage, sizeof(storage), expected, sizeof(expected));

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(
          &reassembly,
          7U,
          (uint8_t *)(uintptr_t)(UINTPTR_MAX - 1U),
          (uint8_t *)(uintptr_t)1U,
          2U),
      TCPIP_L07_INVALID_ARGUMENT);
  expect_empty_reassembly(test, &reassembly);

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

static void test_read_alias_rejections(tcpip_test_context *test) {
  static const uint8_t input[] = {'A', 'B', 'C', 'D'};
  uint8_t data[8] = {0U};
  uint8_t present[8];
  uint8_t expected_data[8];
  uint8_t expected_present[8];
  tcpip_l07_reassembly reassembly;
  size_t accepted = 99U;
  size_t produced = 99U;

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_init(&reassembly, 10U, data, present, sizeof(data)),
      TCPIP_L07_OK);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_push(&reassembly, 10U, input, sizeof(input), &accepted),
      TCPIP_L07_OK);
  memcpy(expected_data, data, sizeof(data));
  memcpy(expected_present, present, sizeof(present));

  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, data, sizeof(data), &produced),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);
  TCPIP_EXPECT_BYTES(test, data, sizeof(data), expected_data, sizeof(expected_data));
  TCPIP_EXPECT_BYTES(test, present, sizeof(present), expected_present, sizeof(expected_present));

  produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, data + 2U, 3U, &produced),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);
  TCPIP_EXPECT_BYTES(test, data, sizeof(data), expected_data, sizeof(expected_data));
  TCPIP_EXPECT_BYTES(test, present, sizeof(present), expected_present, sizeof(expected_present));

  produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, present, sizeof(present), &produced),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);
  TCPIP_EXPECT_BYTES(test, data, sizeof(data), expected_data, sizeof(expected_data));
  TCPIP_EXPECT_BYTES(test, present, sizeof(present), expected_present, sizeof(expected_present));

  produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(&reassembly, present + 2U, 3U, &produced),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);
  TCPIP_EXPECT_BYTES(test, data, sizeof(data), expected_data, sizeof(expected_data));
  TCPIP_EXPECT_BYTES(test, present, sizeof(present), expected_present, sizeof(expected_present));

  produced = 99U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l07_reassembly_read(
          &reassembly, (uint8_t *)(uintptr_t)(UINTPTR_MAX - 1U), 2U, &produced),
      TCPIP_L07_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, produced, 0U);
  TCPIP_EXPECT_SIZE(test, reassembly.read_offset, 0U);
  TCPIP_EXPECT_BYTES(test, data, sizeof(data), expected_data, sizeof(expected_data));
  TCPIP_EXPECT_BYTES(test, present, sizeof(present), expected_present, sizeof(expected_present));
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "lesson 07 TCP state and reliability");
  test_transitions(&test);
  test_reassembly(&test);
  test_initialization_failures(&test);
  test_read_alias_rejections(&test);
  return tcpip_test_finish(&test);
}
