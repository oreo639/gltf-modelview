#ifndef PICCO_SRC_UTIL_TESTS_H_
#define PICCO_SRC_UTIL_TESTS_H_

#include <stdbool.h>

int  utest_end(void);
void utest_prepare(const char *name);
void utest_verify(void);
void utest_error_message(const char *file, int line, const char *func, const char *msg, ...);

#define utest_run(_func, ...) \
	(utest_prepare(""#_func "("#__VA_ARGS__ ")"),((_func)(__VA_ARGS__)),utest_verify())

#define utest_error(...) \
	(utest_error_message(__FILE__, __LINE__, __func__, "" __VA_ARGS__))

#define utest_assert_true(_expr) \
	if (!(_expr)) { utest_error("Assertion failed: "#_expr); return; }

#define utest_assert(_expr, ...) \
	if (!(_expr)) { utest_error("Assertion failed: "#_expr); utest_error(__VA_ARGS__); return; }

#endif
