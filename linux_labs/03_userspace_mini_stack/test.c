#include "lab.h"

#include <string.h>

#include "tcpip/test.h"

static tcpip_linux_l03_scenario tcpip_linux_l03_test_scenario(
    const uint8_t *payload,
    size_t payload_length,
    uint8_t *output,
    size_t output_capacity,
    const tcpip_linux_l03_fault *faults,
    size_t fault_count,
    const tcpip_l11_route *routes,
    size_t route_count) {
  tcpip_linux_l03_scenario scenario;
  memset(&scenario, 0, sizeof(scenario));
  scenario.payload = payload;
  scenario.payload_length = payload_length;
  scenario.output = output;
  scenario.output_capacity = output_capacity;
  scenario.routes = routes;
  scenario.route_count = route_count;
  scenario.source_ipv4[0] = 192U;
  scenario.source_ipv4[1] = 0U;
  scenario.source_ipv4[2] = 2U;
  scenario.source_ipv4[3] = 10U;
  scenario.destination_ipv4[0] = 198U;
  scenario.destination_ipv4[1] = 51U;
  scenario.destination_ipv4[2] = 100U;
  scenario.destination_ipv4[3] = 7U;
  scenario.source_port = 40000U;
  scenario.destination_port = 443U;
  scenario.sender_initial_seq = UINT32_C(1000);
  scenario.receiver_initial_seq = UINT32_C(9000);
  scenario.initial_rto_ms = 100U;
  scenario.max_attempts = 3U;
  scenario.faults = faults;
  scenario.fault_count = fault_count;
  return scenario;
}

static tcpip_l11_route tcpip_linux_l03_default_route(void) {
  tcpip_l11_route route;
  memset(&route, 0, sizeof(route));
  route.prefix_length = 0U;
  route.interface_index = 1U;
  route.metric = 100U;
  return route;
}

static void tcpip_linux_l03_test_clean(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "clean frame";
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, &route, 1U);
  tcpip_linux_l03_result result;

  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.route_index, 0U);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 1U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 0U);
  TCPIP_EXPECT_U32(ctx, result.final_state, TCPIP_L07_TCP_STATE_ESTABLISHED);
  TCPIP_EXPECT_SIZE(ctx, result.reassembled_length, sizeof(payload) - 1U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload) - 1U);
  TCPIP_EXPECT_TRUE(ctx, result.final_frame_diagnostic_length > 0U);
  TCPIP_EXPECT_TRUE(ctx, strstr(result.final_frame_diagnostic, "tcp") != NULL);
  TCPIP_EXPECT_U32(ctx, result.final_frame_report.diagnostics, TCPIP_L12_DIAG_UNSUPPORTED);
}

static void tcpip_linux_l03_test_drop_retransmit(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "retry";
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DROP, 0U}, {TCPIP_LINUX_L03_DELIVER, 0U}};
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), faults, 2U, &route, 1U);
  tcpip_linux_l03_result result;

  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 2U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 1U);
  TCPIP_EXPECT_U32(ctx, (uint32_t)result.virtual_elapsed_ms, 100U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload) - 1U);
}

static void tcpip_linux_l03_test_deadline_is_exclusive(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "deadline";
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DELAY, 100U}, {TCPIP_LINUX_L03_DELIVER, 0U}};
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), faults, 2U, &route, 1U);
  tcpip_linux_l03_result result;

  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 2U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 1U);
  TCPIP_EXPECT_U32(ctx, (uint32_t)result.virtual_elapsed_ms, 100U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload) - 1U);
}

static void tcpip_linux_l03_test_reorder(tcpip_test_context *ctx) {
  uint8_t payload[400U];
  uint8_t output[sizeof(payload)];
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_REORDER, 0U}, {TCPIP_LINUX_L03_DELIVER, 0U}};
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario;
  tcpip_linux_l03_result result;
  size_t index;

  for (index = 0U; index < sizeof(payload); index += 1U) {
    payload[index] = (uint8_t)index;
  }
  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload), output, sizeof(output), faults, 2U, &route, 1U);
  scenario.max_attempts = 4U;
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_U32(ctx, (uint32_t)result.virtual_elapsed_ms, 50U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload));
}

static void tcpip_linux_l03_test_duplicate(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "duplicate is idempotent";
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DELAY, 200U}, {TCPIP_LINUX_L03_DELIVER, 0U}};
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), faults, 2U, &route, 1U);
  tcpip_linux_l03_result result;

  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 2U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 1U);
  TCPIP_EXPECT_U32(ctx, (uint32_t)result.virtual_elapsed_ms, 200U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload) - 1U);
}

static void tcpip_linux_l03_test_exhaustion(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "lost";
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DROP, 0U}, {TCPIP_LINUX_L03_DROP, 0U},
      {TCPIP_LINUX_L03_DROP, 0U}};
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), faults, 3U, &route, 1U);
  tcpip_linux_l03_result result;

  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_RETRY_EXHAUSTED);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 3U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 2U);
  TCPIP_EXPECT_U32(ctx, (uint32_t)result.virtual_elapsed_ms, 700U);
}

static void tcpip_linux_l03_test_route(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "route";
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route routes[2];
  tcpip_linux_l03_scenario scenario;
  tcpip_linux_l03_result result;

  routes[0] = tcpip_linux_l03_default_route();
  memset(&routes[1], 0, sizeof(routes[1]));
  routes[1].network[0] = 198U;
  routes[1].network[1] = 51U;
  routes[1].network[2] = 100U;
  routes[1].prefix_length = 24U;
  routes[1].interface_index = 7U;
  routes[1].metric = 10U;
  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, routes, 2U);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.route_index, 1U);
}

static void tcpip_linux_l03_test_http_diagnostics_observational(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "not http, just bytes";
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, &route, 1U);
  tcpip_linux_l03_result result;

  scenario.destination_port = 80U;
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload) - 1U);
  TCPIP_EXPECT_U32(ctx, result.final_state, TCPIP_L07_TCP_STATE_ESTABLISHED);
  TCPIP_EXPECT_TRUE(ctx, result.final_frame_diagnostic_length > 0U);
  TCPIP_EXPECT_TRUE(
      ctx, (result.final_frame_report.diagnostics & TCPIP_L12_DIAG_MALFORMED) != 0U);
}

static void tcpip_linux_l03_test_event_queue_full_fault_script(tcpip_test_context *ctx) {
  uint8_t payload[TCPIP_LINUX_L03_MAX_PAYLOAD];
  uint8_t output[sizeof(payload)];
  tcpip_linux_l03_fault faults[TCPIP_LINUX_L03_MAX_FAULT_ACTIONS];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario;
  tcpip_linux_l03_result result;
  size_t index;

  for (index = 0U; index < sizeof(payload); index += 1U) {
    payload[index] = (uint8_t)(index + 1U);
  }
  for (index = 0U; index < TCPIP_LINUX_L03_MAX_FAULT_ACTIONS; index += 1U) {
    faults[index].action = TCPIP_LINUX_L03_DELAY;
    faults[index].delay_ms = UINT64_C(1000000) + index;
  }
  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload), output, sizeof(output), faults,
      TCPIP_LINUX_L03_MAX_FAULT_ACTIONS, &route, 1U);
  scenario.max_attempts = 9U;
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 9U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 32U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload));
}

static void tcpip_linux_l03_test_multi_chunk_retransmits(tcpip_test_context *ctx) {
  uint8_t payload[600U];
  uint8_t output[sizeof(payload)];
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DROP, 0U},
      {TCPIP_LINUX_L03_DROP, 0U},
      {TCPIP_LINUX_L03_DROP, 0U}};
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario;
  tcpip_linux_l03_result result;
  size_t index;

  for (index = 0U; index < sizeof(payload); index += 1U) {
    payload[index] = (uint8_t)index;
  }
  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload), output, sizeof(output), faults, 3U, &route, 1U);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_SIZE(ctx, result.attempts, 2U);
  TCPIP_EXPECT_SIZE(ctx, result.retransmits, 3U);
  TCPIP_EXPECT_BYTES(ctx, output, sizeof(output), payload, sizeof(payload));
}

static void tcpip_linux_l03_test_route_error_mapping(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "route errors";
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route;
  tcpip_linux_l03_scenario scenario;
  tcpip_linux_l03_result result;

  memset(&route, 0, sizeof(route));
  route.network[0] = 203U;
  route.network[1] = 0U;
  route.network[2] = 113U;
  route.prefix_length = 24U;
  route.interface_index = 1U;
  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, &route, 1U);
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_ROUTE_NOT_FOUND);

  route = tcpip_linux_l03_default_route();
  route.prefix_length = 33U;
  scenario.routes = &route;
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_MALFORMED);
}

static void tcpip_linux_l03_test_validation_parity(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "validation";
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_fault fault = {(tcpip_linux_l03_fault_action)99, 0U};
  tcpip_linux_l03_scenario scenario;
  tcpip_linux_l03_result result;

  scenario = tcpip_linux_l03_test_scenario(
      payload, 0U, output, sizeof(output), NULL, 0U, &route, 1U);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, result.route_index, SIZE_MAX);
  TCPIP_EXPECT_U32(ctx, result.final_state, TCPIP_L07_TCP_STATE_CLOSED);

  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), &fault, 1U, &route, 1U);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_MALFORMED);
  TCPIP_EXPECT_SIZE(ctx, result.route_index, SIZE_MAX);
}

static void tcpip_linux_l03_test_result_alias_rejected(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "alias";
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, &route, 1U);
  tcpip_linux_l03_result result;
  tcpip_linux_l03_scenario *aliased_scenario = (tcpip_linux_l03_scenario *)&result;

  memset(aliased_scenario, 0, sizeof(*aliased_scenario));
  TCPIP_EXPECT_U32(
      ctx,
      tcpip_linux_l03_run(aliased_scenario, &result),
      TCPIP_LINUX_L03_INVALID_ARGUMENT);

  scenario.output = (uint8_t *)&result;
  scenario.output_capacity = sizeof(result);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_INVALID_ARGUMENT);

  scenario = tcpip_linux_l03_test_scenario(
      (const uint8_t *)&result, sizeof(result), output, sizeof(output), NULL, 0U, &route, 1U);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_INVALID_ARGUMENT);

  scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, &route, 1U);
  scenario.output = (uint8_t *)&result + 1U;
  scenario.output_capacity = 2U;
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_INVALID_ARGUMENT);
}

static void tcpip_linux_l03_test_exhaustion_state(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "state";
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DROP, 0U}, {TCPIP_LINUX_L03_DROP, 0U}};
  uint8_t output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), faults, 2U, &route, 1U);
  tcpip_linux_l03_result result;

  scenario.max_attempts = 2U;
  TCPIP_EXPECT_U32(
      ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_RETRY_EXHAUSTED);
  TCPIP_EXPECT_U32(ctx, result.final_state, TCPIP_L07_TCP_STATE_ESTABLISHED);
}

static void tcpip_linux_l03_test_capacity(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "too large for output";
  uint8_t output[2U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario scenario = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, output, sizeof(output), NULL, 0U, &route, 1U);
  tcpip_linux_l03_result result;

  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&scenario, &result), TCPIP_LINUX_L03_CAPACITY);
  TCPIP_EXPECT_SIZE(ctx, result.route_index, SIZE_MAX);
}

static void tcpip_linux_l03_test_deterministic(tcpip_test_context *ctx) {
  static const uint8_t payload[] = "repeat";
  const tcpip_linux_l03_fault faults[] = {
      {TCPIP_LINUX_L03_DROP, 0U}, {TCPIP_LINUX_L03_DELIVER, 0U}};
  uint8_t first_output[sizeof(payload) - 1U];
  uint8_t second_output[sizeof(payload) - 1U];
  tcpip_l11_route route = tcpip_linux_l03_default_route();
  tcpip_linux_l03_scenario first = tcpip_linux_l03_test_scenario(
      payload, sizeof(payload) - 1U, first_output, sizeof(first_output), faults, 2U, &route, 1U);
  tcpip_linux_l03_scenario second = first;
  tcpip_linux_l03_result first_result;
  tcpip_linux_l03_result second_result;
  second.output = second_output;

  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&first, &first_result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_U32(ctx, tcpip_linux_l03_run(&second, &second_result), TCPIP_LINUX_L03_OK);
  TCPIP_EXPECT_BYTES(
      ctx, (const uint8_t *)&first_result, sizeof(first_result),
      (const uint8_t *)&second_result, sizeof(second_result));
  TCPIP_EXPECT_BYTES(ctx, first_output, sizeof(first_output), second_output, sizeof(second_output));
}

int main(void) {
  tcpip_test_context ctx;
  tcpip_test_begin(&ctx, "linux lab 03 userspace mini stack");
  tcpip_linux_l03_test_clean(&ctx);
  tcpip_linux_l03_test_drop_retransmit(&ctx);
  tcpip_linux_l03_test_deadline_is_exclusive(&ctx);
  tcpip_linux_l03_test_reorder(&ctx);
  tcpip_linux_l03_test_duplicate(&ctx);
  tcpip_linux_l03_test_exhaustion(&ctx);
  tcpip_linux_l03_test_route(&ctx);
  tcpip_linux_l03_test_http_diagnostics_observational(&ctx);
  tcpip_linux_l03_test_event_queue_full_fault_script(&ctx);
  tcpip_linux_l03_test_multi_chunk_retransmits(&ctx);
  tcpip_linux_l03_test_route_error_mapping(&ctx);
  tcpip_linux_l03_test_validation_parity(&ctx);
  tcpip_linux_l03_test_result_alias_rejected(&ctx);
  tcpip_linux_l03_test_exhaustion_state(&ctx);
  tcpip_linux_l03_test_capacity(&ctx);
  tcpip_linux_l03_test_deterministic(&ctx);
  return tcpip_test_finish(&ctx);
}
