#include "lesson.h"
#include "tcpip/test.h"

#include <stdint.h>
#include <string.h>

#define TCPIP_L11_IP(a, b, c, d) {(uint8_t)(a), (uint8_t)(b), (uint8_t)(c), (uint8_t)(d)}

static int tcpip_l11_tuple_is_zero(const tcpip_l11_tuple *tuple) {
  const tcpip_l11_tuple zero = {0};
  return memcmp(tuple, &zero, sizeof(zero)) == 0;
}

static tcpip_l11_tuple tcpip_l11_make_tuple(
    uint8_t protocol,
    const uint8_t source_ip[4],
    uint16_t source_port,
    const uint8_t destination_ip[4],
    uint16_t destination_port) {
  tcpip_l11_tuple tuple;
  tuple.protocol = protocol;
  memcpy(tuple.source_ip, source_ip, sizeof(tuple.source_ip));
  memcpy(tuple.destination_ip, destination_ip, sizeof(tuple.destination_ip));
  tuple.source_port = source_port;
  tuple.destination_port = destination_port;
  return tuple;
}

static void tcpip_l11_test_route_selection(tcpip_test_context *test) {
  static const tcpip_l11_route routes[] = {
      {{0u, 0u, 0u, 0u}, 0u, {192u, 0u, 2u, 1u}, 1u, 100u},
      {{10u, 0u, 0u, 0u}, 8u, {192u, 0u, 2u, 2u}, 2u, 20u},
      {{10u, 23u, 0u, 0u}, 16u, {192u, 0u, 2u, 3u}, 3u, 40u},
      {{10u, 23u, 0u, 0u}, 16u, {192u, 0u, 2u, 4u}, 4u, 10u},
      {{10u, 23u, 0u, 0u}, 16u, {192u, 0u, 2u, 5u}, 5u, 10u},
      {{10u, 23u, 42u, 0u}, 24u, {192u, 0u, 2u, 6u}, 6u, 500u},
  };
  static const uint8_t specific[4] = TCPIP_L11_IP(10, 23, 42, 99);
  static const uint8_t subnet[4] = TCPIP_L11_IP(10, 23, 7, 9);
  static const uint8_t fallback[4] = TCPIP_L11_IP(203, 0, 113, 7);
  size_t index = 999u;

  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(
          routes, sizeof(routes) / sizeof(routes[0]), specific, &index),
      TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(test, index, 5u);

  index = 999u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(
          routes, sizeof(routes) / sizeof(routes[0]), subnet, &index),
      TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(test, index, 3u);

  index = 999u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(
          routes, sizeof(routes) / sizeof(routes[0]), fallback, &index),
      TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(test, index, 0u);
}

static void tcpip_l11_test_route_errors(tcpip_test_context *test) {
  static const uint8_t destination[4] = TCPIP_L11_IP(10, 1, 2, 3);
  static const tcpip_l11_route no_match = {
      {192u, 168u, 0u, 0u}, 16u, {192u, 0u, 2u, 1u}, 1u, 1u};
  static const tcpip_l11_route host_bits = {
      {10u, 1u, 0u, 1u}, 24u, {192u, 0u, 2u, 1u}, 1u, 1u};
  static const tcpip_l11_route long_prefix = {
      {10u, 1u, 2u, 3u}, 33u, {192u, 0u, 2u, 1u}, 1u, 1u};
  size_t index = 44u;

  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(&no_match, 1u, destination, &index),
      TCPIP_L11_TRUNCATED);
  TCPIP_EXPECT_SIZE(test, index, SIZE_MAX);
  index = 44u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(&host_bits, 1u, destination, &index),
      TCPIP_L11_MALFORMED);
  TCPIP_EXPECT_SIZE(test, index, SIZE_MAX);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(&long_prefix, 1u, destination, &index),
      TCPIP_L11_MALFORMED);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_longest_prefix(NULL, 1u, destination, &index),
      TCPIP_L11_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, index, SIZE_MAX);
}

static int tcpip_l11_init_nat(
    tcpip_test_context *test,
    tcpip_l11_nat *nat,
    tcpip_l11_nat_mapping *storage,
    size_t capacity,
    uint16_t first_port,
    const uint8_t public_ip[4]) {
  memset(storage, 0xa5, capacity * sizeof(*storage));
  const tcpip_l11_status status =
      tcpip_l11_nat_init(nat, first_port, public_ip, storage, capacity);
  TCPIP_EXPECT_U32(test, status, TCPIP_L11_OK);
  if (status != TCPIP_L11_OK) {
    return 0;
  }
  for (size_t index = 0u; index < capacity; index += 1u) {
    TCPIP_EXPECT_U32(test, storage[index].active, 0u);
  }
  return 1;
}

static void tcpip_l11_test_outbound_reuse_and_collision(tcpip_test_context *test) {
  static const uint8_t public_ip[4] = TCPIP_L11_IP(198, 51, 100, 9);
  static const uint8_t private_a[4] = TCPIP_L11_IP(10, 0, 0, 2);
  static const uint8_t private_b[4] = TCPIP_L11_IP(10, 0, 0, 3);
  static const uint8_t remote_a[4] = TCPIP_L11_IP(203, 0, 113, 10);
  static const uint8_t remote_b[4] = TCPIP_L11_IP(203, 0, 113, 11);
  tcpip_l11_nat_mapping storage[4];
  tcpip_l11_nat nat;
  if (!tcpip_l11_init_nat(test, &nat, storage, 4u, 40000u, public_ip)) {
    return;
  }

  const tcpip_l11_tuple first =
      tcpip_l11_make_tuple(6u, private_a, 51000u, remote_a, 443u);
  tcpip_l11_tuple translated;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 10u, &first, &translated),
      TCPIP_L11_OK);
  TCPIP_EXPECT_BYTES(test, translated.source_ip, 4u, public_ip, 4u);
  TCPIP_EXPECT_U32(test, translated.source_port, 40000u);
  TCPIP_EXPECT_BYTES(test, translated.destination_ip, 4u, remote_a, 4u);
  TCPIP_EXPECT_U32(test, translated.destination_port, 443u);
  TCPIP_EXPECT_U32(test, storage[0].active, 1u);
  TCPIP_EXPECT_U32(test, storage[0].last_used, 10u);

  tcpip_l11_tuple aliased = first;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 20u, &aliased, &aliased),
      TCPIP_L11_OK);
  TCPIP_EXPECT_U32(test, aliased.source_port, 40000u);
  TCPIP_EXPECT_U32(test, storage[0].last_used, 20u);
  TCPIP_EXPECT_U32(test, storage[1].active, 0u);

  const tcpip_l11_tuple second =
      tcpip_l11_make_tuple(17u, private_b, 51000u, remote_b, 53u);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 21u, &second, &translated),
      TCPIP_L11_OK);
  TCPIP_EXPECT_U32(test, translated.source_port, 40001u);
  TCPIP_EXPECT_TRUE(test, storage[0].public_port != storage[1].public_port);

  const tcpip_l11_tuple endpoint_changed =
      tcpip_l11_make_tuple(6u, private_a, 51000u, remote_b, 443u);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 22u, &endpoint_changed, &translated),
      TCPIP_L11_OK);
  TCPIP_EXPECT_U32(test, translated.source_port, 40002u);
}

static void tcpip_l11_test_reverse_only(tcpip_test_context *test) {
  static const uint8_t public_ip[4] = TCPIP_L11_IP(198, 51, 100, 20);
  static const uint8_t private_ip[4] = TCPIP_L11_IP(10, 4, 0, 7);
  static const uint8_t remote_ip[4] = TCPIP_L11_IP(203, 0, 113, 80);
  static const uint8_t stranger_ip[4] = TCPIP_L11_IP(203, 0, 113, 81);
  tcpip_l11_nat_mapping storage[2];
  tcpip_l11_nat nat;
  if (!tcpip_l11_init_nat(test, &nat, storage, 2u, 45000u, public_ip)) {
    return;
  }

  tcpip_l11_tuple incoming =
      tcpip_l11_make_tuple(6u, remote_ip, 443u, public_ip, 45000u);
  tcpip_l11_tuple translated;
  memset(&translated, 0xa5, sizeof(translated));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_inbound(&nat, 1u, &incoming, &translated),
      TCPIP_L11_TRUNCATED);
  TCPIP_EXPECT_TRUE(test, tcpip_l11_tuple_is_zero(&translated));
  TCPIP_EXPECT_U32(test, storage[0].active, 0u);

  const tcpip_l11_tuple outgoing =
      tcpip_l11_make_tuple(6u, private_ip, 52000u, remote_ip, 443u);
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 2u, &outgoing, &translated),
      TCPIP_L11_OK);

  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_inbound(&nat, 3u, &incoming, &translated),
      TCPIP_L11_OK);
  TCPIP_EXPECT_BYTES(test, translated.destination_ip, 4u, private_ip, 4u);
  TCPIP_EXPECT_U32(test, translated.destination_port, 52000u);
  TCPIP_EXPECT_BYTES(test, translated.source_ip, 4u, remote_ip, 4u);
  TCPIP_EXPECT_U32(test, storage[0].last_used, 3u);

  incoming.source_ip[3] = stranger_ip[3];
  memset(&translated, 0xa5, sizeof(translated));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_inbound(&nat, 4u, &incoming, &translated),
      TCPIP_L11_TRUNCATED);
  TCPIP_EXPECT_TRUE(test, tcpip_l11_tuple_is_zero(&translated));
  TCPIP_EXPECT_U32(test, storage[0].last_used, 3u);
}

static void tcpip_l11_test_capacity_and_no_partial_commit(tcpip_test_context *test) {
  static const uint8_t public_ip[4] = TCPIP_L11_IP(192, 0, 2, 44);
  static const uint8_t private_ip[4] = TCPIP_L11_IP(10, 0, 0, 4);
  static const uint8_t remote_a[4] = TCPIP_L11_IP(198, 51, 100, 1);
  static const uint8_t remote_b[4] = TCPIP_L11_IP(198, 51, 100, 2);
  tcpip_l11_nat_mapping storage[1];
  tcpip_l11_nat nat;
  if (!tcpip_l11_init_nat(test, &nat, storage, 1u, 65535u, public_ip)) {
    return;
  }

  const tcpip_l11_tuple first =
      tcpip_l11_make_tuple(17u, private_ip, 1234u, remote_a, 53u);
  const tcpip_l11_tuple second =
      tcpip_l11_make_tuple(17u, private_ip, 1234u, remote_b, 53u);
  tcpip_l11_tuple translated;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 10u, &first, &translated),
      TCPIP_L11_OK);
  const tcpip_l11_nat_mapping snapshot = storage[0];
  memset(&translated, 0xa5, sizeof(translated));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 11u, &second, &translated),
      TCPIP_L11_CAPACITY);
  TCPIP_EXPECT_TRUE(test, tcpip_l11_tuple_is_zero(&translated));
  TCPIP_EXPECT_TRUE(test, memcmp(&storage[0], &snapshot, sizeof(snapshot)) == 0);

  tcpip_l11_nat_mapping port_storage[2];
  tcpip_l11_nat port_nat;
  if (!tcpip_l11_init_nat(test, &port_nat, port_storage, 2u, 65535u, public_ip)) {
    return;
  }
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&port_nat, 20u, &first, &translated),
      TCPIP_L11_OK);
  TCPIP_EXPECT_U32(test, translated.source_port, 65535u);
  memset(&translated, 0xa5, sizeof(translated));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&port_nat, 21u, &second, &translated),
      TCPIP_L11_CAPACITY);
  TCPIP_EXPECT_TRUE(test, tcpip_l11_tuple_is_zero(&translated));
  TCPIP_EXPECT_U32(test, port_storage[1].active, 0u);
}

static void tcpip_l11_test_expiry_and_reuse(tcpip_test_context *test) {
  static const uint8_t public_ip[4] = TCPIP_L11_IP(192, 0, 2, 90);
  static const uint8_t private_ip[4] = TCPIP_L11_IP(10, 8, 0, 2);
  static const uint8_t remote_a[4] = TCPIP_L11_IP(203, 0, 113, 1);
  static const uint8_t remote_b[4] = TCPIP_L11_IP(203, 0, 113, 2);
  tcpip_l11_nat_mapping storage[2];
  tcpip_l11_nat nat;
  if (!tcpip_l11_init_nat(test, &nat, storage, 2u, 50000u, public_ip)) {
    return;
  }
  const tcpip_l11_tuple first =
      tcpip_l11_make_tuple(6u, private_ip, 3000u, remote_a, 80u);
  const tcpip_l11_tuple second =
      tcpip_l11_make_tuple(6u, private_ip, 3001u, remote_b, 80u);
  tcpip_l11_tuple translated;
  (void)tcpip_l11_nat_translate_outbound(&nat, 100u, &first, &translated);
  (void)tcpip_l11_nat_translate_outbound(&nat, 105u, &second, &translated);

  size_t expired = 99u;
  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_expire(&nat, 109u, 10u, &expired), TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(test, expired, 0u);
  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_expire(&nat, 110u, 10u, &expired), TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(test, expired, 1u);
  TCPIP_EXPECT_U32(test, storage[0].active, 0u);
  TCPIP_EXPECT_U32(test, storage[1].active, 1u);

  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 111u, &first, &translated),
      TCPIP_L11_OK);
  TCPIP_EXPECT_U32(test, translated.source_port, 50000u);
  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_expire(&nat, 100u, 1u, &expired), TCPIP_L11_OK);
  TCPIP_EXPECT_SIZE(test, expired, 0u);
}

static void tcpip_l11_test_validation(tcpip_test_context *test) {
  static const uint8_t public_ip[4] = TCPIP_L11_IP(192, 0, 2, 1);
  static const uint8_t private_ip[4] = TCPIP_L11_IP(10, 0, 0, 1);
  static const uint8_t remote_ip[4] = TCPIP_L11_IP(203, 0, 113, 1);
  tcpip_l11_nat_mapping storage[2];
  tcpip_l11_nat nat;
  tcpip_l11_tuple out;
  size_t count = 88u;

  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_init(NULL, 1000u, public_ip, storage, 2u),
      TCPIP_L11_INVALID_ARGUMENT);
  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_init(&nat, 0u, public_ip, storage, 2u),
      TCPIP_L11_INVALID_ARGUMENT);
  if (!tcpip_l11_init_nat(test, &nat, storage, 2u, 1000u, public_ip)) {
    return;
  }

  tcpip_l11_tuple invalid =
      tcpip_l11_make_tuple(1u, private_ip, 100u, remote_ip, 200u);
  memset(&out, 0xa5, sizeof(out));
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 0u, &invalid, &out),
      TCPIP_L11_MALFORMED);
  TCPIP_EXPECT_TRUE(test, tcpip_l11_tuple_is_zero(&out));
  TCPIP_EXPECT_U32(test, storage[0].active, 0u);

  invalid.protocol = 6u;
  invalid.source_port = 0u;
  TCPIP_EXPECT_U32(
      test,
      tcpip_l11_nat_translate_outbound(&nat, 0u, &invalid, &out),
      TCPIP_L11_MALFORMED);
  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_expire(NULL, 1u, 1u, &count), TCPIP_L11_INVALID_ARGUMENT);
  TCPIP_EXPECT_SIZE(test, count, 0u);

  const tcpip_l11_tuple valid =
      tcpip_l11_make_tuple(6u, private_ip, 123u, remote_ip, 80u);
  (void)tcpip_l11_nat_translate_outbound(&nat, 1u, &valid, &out);
  storage[1] = storage[0];
  const tcpip_l11_nat_mapping first_snapshot = storage[0];
  const tcpip_l11_nat_mapping second_snapshot = storage[1];
  count = 88u;
  TCPIP_EXPECT_U32(
      test, tcpip_l11_nat_expire(&nat, 100u, 1u, &count), TCPIP_L11_MALFORMED);
  TCPIP_EXPECT_SIZE(test, count, 0u);
  TCPIP_EXPECT_TRUE(test, memcmp(&storage[0], &first_snapshot, sizeof(storage[0])) == 0);
  TCPIP_EXPECT_TRUE(test, memcmp(&storage[1], &second_snapshot, sizeof(storage[1])) == 0);
}

int main(void) {
  tcpip_test_context test;
  tcpip_test_begin(&test, "lesson 11 routing and NAT");
  tcpip_l11_test_route_selection(&test);
  tcpip_l11_test_route_errors(&test);
  tcpip_l11_test_outbound_reuse_and_collision(&test);
  tcpip_l11_test_reverse_only(&test);
  tcpip_l11_test_capacity_and_no_partial_commit(&test);
  tcpip_l11_test_expiry_and_reuse(&test);
  tcpip_l11_test_validation(&test);
  return tcpip_test_finish(&test);
}
