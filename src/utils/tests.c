#include <stdio.h>
#include <stdarg.h>

#include "tests.h"

enum test_runner_state {
	TEST_PASSED,
	TEST_FAILED,
};

static enum test_runner_state test_runner_state;

static int failed = 0;
static int passed = 0;

int utest_end(void) {
	return !!failed;
}

void utest_prepare(const char *name) {
	fprintf(stderr, "Running test %s...\n", name);
	test_runner_state = TEST_PASSED;
}

void utest_verify(void) {
	if (test_runner_state == TEST_FAILED) {
		fprintf(stderr, "Failed. Look above for more details.\n");
		++failed;
	} else if (test_runner_state == TEST_PASSED) {
		++passed;
	}
}

void utest_error_message(const char *file, int line, const char *func, const char *msg, ...) {
	fprintf(stderr, "%s:%d: %s: error: ", file, line, func);

	va_list va = {0};
	va_start(va, msg);
	vfprintf(stderr, msg, va);
	va_end(va);

	fputs("\n", stderr);
	test_runner_state = TEST_FAILED;
}
