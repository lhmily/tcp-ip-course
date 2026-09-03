#ifndef TCPIP_TEST_H
#define TCPIP_TEST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tcpip_test_context {
  const char *name;
  unsigned failures;
} tcpip_test_context;

void tcpip_test_begin(tcpip_test_context *ctx, const char *name);
int tcpip_test_finish(const tcpip_test_context *ctx);
void tcpip_test_fail(tcpip_test_context *ctx, const char *file, int line, const char *message);
void tcpip_test_expect_true(
    tcpip_test_context *ctx, const char *file, int line, int condition, const char *expression);
void tcpip_test_expect_u16(
    tcpip_test_context *ctx, const char *file, int line, uint16_t actual, uint16_t expected);
void tcpip_test_expect_u32(
    tcpip_test_context *ctx, const char *file, int line, uint32_t actual, uint32_t expected);
void tcpip_test_expect_size(
    tcpip_test_context *ctx, const char *file, int line, size_t actual, size_t expected);
void tcpip_test_expect_bytes(
    tcpip_test_context *ctx,
    const char *file,
    int line,
    const uint8_t *actual,
    size_t actual_len,
    const uint8_t *expected,
    size_t expected_len);
void tcpip_test_expect_cstr(
    tcpip_test_context *ctx, const char *file, int line, const char *actual, const char *expected);

#define TCPIP_EXPECT_TRUE(ctx, condition) \
  tcpip_test_expect_true((ctx), __FILE__, __LINE__, (condition), #condition)
#define TCPIP_EXPECT_U16(ctx, actual, expected) \
  tcpip_test_expect_u16((ctx), __FILE__, __LINE__, (uint16_t)(actual), (uint16_t)(expected))
#define TCPIP_EXPECT_U32(ctx, actual, expected) \
  tcpip_test_expect_u32((ctx), __FILE__, __LINE__, (uint32_t)(actual), (uint32_t)(expected))
#define TCPIP_EXPECT_SIZE(ctx, actual, expected) \
  tcpip_test_expect_size((ctx), __FILE__, __LINE__, (size_t)(actual), (size_t)(expected))
#define TCPIP_EXPECT_BYTES(ctx, actual, actual_len, expected, expected_len) \
  tcpip_test_expect_bytes((ctx), __FILE__, __LINE__, (actual), (actual_len), (expected), (expected_len))
#define TCPIP_EXPECT_CSTR(ctx, actual, expected) \
  tcpip_test_expect_cstr((ctx), __FILE__, __LINE__, (actual), (expected))
#define TCPIP_FAIL(ctx, message) tcpip_test_fail((ctx), __FILE__, __LINE__, (message))

#ifdef __cplusplus
}
#endif

#endif
