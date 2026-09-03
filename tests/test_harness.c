#include "tcpip/test.h"

#include <stdio.h>
#include <string.h>

void tcpip_test_begin(tcpip_test_context *ctx, const char *name) {
  ctx->name = name;
  ctx->failures = 0;
}

int tcpip_test_finish(const tcpip_test_context *ctx) {
  if (ctx->failures == 0) {
    printf("ok - %s\n", ctx->name);
    return 0;
  }
  printf("not ok - %s (%u failure%s)\n", ctx->name, ctx->failures, ctx->failures == 1 ? "" : "s");
  return 1;
}

void tcpip_test_fail(tcpip_test_context *ctx, const char *file, int line, const char *message) {
  ctx->failures += 1;
  fprintf(stderr, "%s:%d: %s\n", file, line, message);
}

void tcpip_test_expect_true(
    tcpip_test_context *ctx, const char *file, int line, int condition, const char *expression) {
  if (!condition) {
    ctx->failures += 1;
    fprintf(stderr, "%s:%d: expected true: %s\n", file, line, expression);
  }
}

void tcpip_test_expect_u16(
    tcpip_test_context *ctx, const char *file, int line, uint16_t actual, uint16_t expected) {
  if (actual != expected) {
    ctx->failures += 1;
    fprintf(stderr, "%s:%d: expected 0x%04x, got 0x%04x\n", file, line, expected, actual);
  }
}

void tcpip_test_expect_u32(
    tcpip_test_context *ctx, const char *file, int line, uint32_t actual, uint32_t expected) {
  if (actual != expected) {
    ctx->failures += 1;
    fprintf(stderr, "%s:%d: expected 0x%08x, got 0x%08x\n", file, line, expected, actual);
  }
}

void tcpip_test_expect_size(
    tcpip_test_context *ctx, const char *file, int line, size_t actual, size_t expected) {
  if (actual != expected) {
    ctx->failures += 1;
    fprintf(stderr, "%s:%d: expected %zu, got %zu\n", file, line, expected, actual);
  }
}

void tcpip_test_expect_bytes(
    tcpip_test_context *ctx,
    const char *file,
    int line,
    const uint8_t *actual,
    size_t actual_len,
    const uint8_t *expected,
    size_t expected_len) {
  if (actual_len != expected_len) {
    ctx->failures += 1;
    fprintf(stderr, "%s:%d: expected %zu bytes, got %zu bytes\n", file, line, expected_len, actual_len);
    return;
  }
  for (size_t index = 0; index < expected_len; index += 1) {
    if (actual[index] != expected[index]) {
      ctx->failures += 1;
      fprintf(
          stderr,
          "%s:%d: byte %zu: expected 0x%02x, got 0x%02x\n",
          file,
          line,
          index,
          expected[index],
          actual[index]);
      return;
    }
  }
}

void tcpip_test_expect_cstr(
    tcpip_test_context *ctx, const char *file, int line, const char *actual, const char *expected) {
  if (strcmp(actual, expected) != 0) {
    ctx->failures += 1;
    fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", file, line, expected, actual);
  }
}
