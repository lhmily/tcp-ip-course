#include "lab.h"
#include "tcpip/test.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct tcp_pair {
  int listener;
  int client;
  int server;
} tcp_pair;

static void close_pair(tcp_pair *pair) {
  if (pair->server >= 0) {
    (void)close(pair->server);
  }
  if (pair->client >= 0) {
    (void)close(pair->client);
  }
  if (pair->listener >= 0) {
    (void)close(pair->listener);
  }
  pair->listener = -1;
  pair->client = -1;
  pair->server = -1;
}

static int open_pair(tcp_pair *pair) {
  struct sockaddr_in address;
  socklen_t length = (socklen_t)sizeof(address);
  int option = 1;

  pair->listener = -1;
  pair->client = -1;
  pair->server = -1;
  pair->listener = socket(AF_INET, SOCK_STREAM, 0);
  if (pair->listener < 0) {
    return -1;
  }
  (void)setsockopt(
      pair->listener, SOL_SOCKET, SO_REUSEADDR, &option, (socklen_t)sizeof(option));
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(0U);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(pair->listener, (struct sockaddr *)&address, sizeof(address)) < 0 ||
      listen(pair->listener, 1) < 0 ||
      getsockname(pair->listener, (struct sockaddr *)&address, &length) < 0) {
    close_pair(pair);
    return -1;
  }
  pair->client = socket(AF_INET, SOCK_STREAM, 0);
  if (pair->client < 0 ||
      connect(pair->client, (struct sockaddr *)&address, sizeof(address)) < 0) {
    close_pair(pair);
    return -1;
  }
  pair->server = accept(pair->listener, NULL, NULL);
  if (pair->server < 0) {
    close_pair(pair);
    return -1;
  }
  return 0;
}

static void expect_established_snapshot(
    tcpip_test_context *test,
    const tcpip_linux_l02_snapshot *snapshot) {
  tcpip_linux_l02_model_state state = TCPIP_LINUX_L02_MODEL_CLOSED;
  tcpip_linux_l02_violations violations;

  TCPIP_EXPECT_TRUE(test, (snapshot->present & TCPIP_LINUX_L02_HAS_STATE) != 0U);
  TCPIP_EXPECT_TRUE(test, (snapshot->present & TCPIP_LINUX_L02_HAS_SND_MSS) != 0U);
  TCPIP_EXPECT_TRUE(test, (snapshot->present & TCPIP_LINUX_L02_HAS_RCV_MSS) != 0U);
  TCPIP_EXPECT_TRUE(test, (snapshot->present & TCPIP_LINUX_L02_HAS_RTT_US) != 0U);
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_map_state(snapshot->state, &state), TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(test, state, TCPIP_LINUX_L02_MODEL_ESTABLISHED);
  TCPIP_EXPECT_TRUE(test, snapshot->snd_mss > 0U);
  TCPIP_EXPECT_TRUE(test, snapshot->rcv_mss > 0U);
  TCPIP_EXPECT_TRUE(test, snapshot->rtt_us > 0U);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_check_invariants(snapshot, &violations),
      TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(test, violations.bits, 0U);
  TCPIP_EXPECT_U32(test, violations.count, 0U);
}

static void test_loopback_capture(tcpip_test_context *test) {
  tcp_pair pair;
  tcpip_linux_l02_snapshot client_snapshot;
  tcpip_linux_l02_snapshot server_snapshot;
  static const uint8_t payload[] = {'t', 'c', 'p', '-', 'i', 'n', 'f', 'o'};
  uint8_t received[sizeof(payload)] = {0U};

  if (open_pair(&pair) < 0) {
    TCPIP_FAIL(test, "loopback TCP setup failed");
    return;
  }
  TCPIP_EXPECT_SIZE(
      test,
      (size_t)send(pair.client, payload, sizeof(payload), MSG_NOSIGNAL),
      sizeof(payload));
  TCPIP_EXPECT_SIZE(
      test, (size_t)recv(pair.server, received, sizeof(received), MSG_WAITALL), sizeof(payload));
  TCPIP_EXPECT_BYTES(test, received, sizeof(received), payload, sizeof(payload));

  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_capture(pair.client, &client_snapshot),
      TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_capture(pair.server, &server_snapshot),
      TCPIP_LINUX_L02_OK);
  expect_established_snapshot(test, &client_snapshot);
  expect_established_snapshot(test, &server_snapshot);
  close_pair(&pair);
}

static void test_listen_and_mapping(tcpip_test_context *test) {
  tcp_pair pair;
  tcpip_linux_l02_snapshot snapshot;
  tcpip_linux_l02_model_state state;

  if (open_pair(&pair) < 0) {
    TCPIP_FAIL(test, "loopback TCP setup failed");
    return;
  }
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_capture(pair.listener, &snapshot),
      TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_map_state(snapshot.state, &state),
      TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(test, state, TCPIP_LINUX_L02_MODEL_LISTEN);

  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_map_state(TCP_CLOSE, &state), TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(test, state, TCPIP_LINUX_L02_MODEL_CLOSED);
  state = TCPIP_LINUX_L02_MODEL_TIME_WAIT;
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_map_state(UINT8_C(255), &state), TCPIP_LINUX_L02_UNSUPPORTED);
  TCPIP_EXPECT_U32(test, state, TCPIP_LINUX_L02_MODEL_CLOSED);
  close_pair(&pair);
}

static void test_rejection_and_initialization(tcpip_test_context *test) {
  int sockets[2];
  tcpip_linux_l02_snapshot snapshot;
  tcpip_linux_l02_violations violations;
  tcpip_linux_l02_model_state state = TCPIP_LINUX_L02_MODEL_TIME_WAIT;
  static const tcpip_linux_l02_snapshot empty_snapshot = {0U};

  memset(&snapshot, 0xa5, sizeof(snapshot));
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_capture(-1, &snapshot), TCPIP_LINUX_L02_INVALID_ARGUMENT);
  TCPIP_EXPECT_BYTES(
      test,
      (const uint8_t *)&snapshot,
      sizeof(snapshot),
      (const uint8_t *)&empty_snapshot,
      sizeof(empty_snapshot));
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_capture(0, NULL), TCPIP_LINUX_L02_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_map_state(TCP_ESTABLISHED, NULL), TCPIP_LINUX_L02_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_check_invariants(NULL, &violations),
      TCPIP_LINUX_L02_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_check_invariants(&snapshot, NULL),
      TCPIP_LINUX_L02_INVALID_ARGUMENT);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    TCPIP_FAIL(test, "Unix socketpair failed");
    return;
  }
  memset(&snapshot, 0xa5, sizeof(snapshot));
  TCPIP_EXPECT_U32(
      test, tcpip_linux_l02_capture(sockets[0], &snapshot), TCPIP_LINUX_L02_UNSUPPORTED);
  TCPIP_EXPECT_BYTES(
      test,
      (const uint8_t *)&snapshot,
      sizeof(snapshot),
      (const uint8_t *)&empty_snapshot,
      sizeof(empty_snapshot));
  (void)close(sockets[0]);
  (void)close(sockets[1]);

  memset(&violations, 0xa5, sizeof(violations));
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_check_invariants(&empty_snapshot, &violations),
      TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(
      test,
      violations.bits,
      TCPIP_LINUX_L02_VIOLATION_MISSING_STATE);
  TCPIP_EXPECT_U32(test, violations.count, 1U);
  TCPIP_EXPECT_U32(test, state, TCPIP_LINUX_L02_MODEL_TIME_WAIT);
}

static void test_synthetic_invariants(tcpip_test_context *test) {
  tcpip_linux_l02_snapshot snapshot;
  tcpip_linux_l02_violations violations;

  memset(&snapshot, 0, sizeof(snapshot));
  snapshot.present = TCPIP_LINUX_L02_HAS_STATE | TCPIP_LINUX_L02_HAS_SND_MSS |
                     TCPIP_LINUX_L02_HAS_RCV_MSS | TCPIP_LINUX_L02_HAS_RTT_US |
                     TCPIP_LINUX_L02_HAS_LOST;
  snapshot.state = UINT8_C(255);
  snapshot.lost = 1U;
  TCPIP_EXPECT_U32(
      test,
      tcpip_linux_l02_check_invariants(&snapshot, &violations),
      TCPIP_LINUX_L02_OK);
  TCPIP_EXPECT_U32(test, violations.count, 5U);
  TCPIP_EXPECT_TRUE(
      test,
      (violations.bits & TCPIP_LINUX_L02_VIOLATION_UNKNOWN_STATE) != 0U);
  TCPIP_EXPECT_TRUE(
      test,
      (violations.bits & TCPIP_LINUX_L02_VIOLATION_ZERO_SND_MSS) != 0U);
  TCPIP_EXPECT_TRUE(
      test,
      (violations.bits & TCPIP_LINUX_L02_VIOLATION_ZERO_RCV_MSS) != 0U);
  TCPIP_EXPECT_TRUE(
      test,
      (violations.bits & TCPIP_LINUX_L02_VIOLATION_ZERO_RTT) != 0U);
  TCPIP_EXPECT_TRUE(
      test,
      (violations.bits & TCPIP_LINUX_L02_VIOLATION_LOST_WITHOUT_RETRANS) != 0U);
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "Linux lab 02 TCP_INFO");
  test_loopback_capture(&test);
  test_listen_and_mapping(&test);
  test_rejection_and_initialization(&test);
  test_synthetic_invariants(&test);
  return tcpip_test_finish(&test);
}
