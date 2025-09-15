#include <stdio.h>

#include <string_buffer.h>
#include <tests.h>

static void test_sb_new_unterminated(char *message) {
	struct string_buffer sb = string_buffer_new(message);
	utest_assert_true(memcmp(sb.buf, message, strlen(message)) == 0);
	utest_assert_true(sb.buf_len == strlen(message));
	string_buffer_free(&sb);
}

static void test_sb_new_terminated(char *message) {
	struct string_buffer sb = string_buffer_new(message);
	const char *buf = string_buffer_terminate(&sb);
	utest_assert_true(strcmp(buf, message) == 0);
	utest_assert_true(strcmp(buf, sb.buf) == 0);
	utest_assert_true(strlen(message)==sb.buf_len);
	string_buffer_free(&sb);
}

static void test_sb_append_unterminated(void) {
	struct string_buffer sb = string_buffer_new(NULL);
	string_buffer_append(&sb, "Hello world");
	utest_assert_true(memcmp(sb.buf, "Hello world", 11) == 0);
	string_buffer_append(&sb, "...");
	utest_assert_true(memcmp(sb.buf, "Hello world...", 14) == 0);
	string_buffer_free(&sb);
}

static void test_sb_append_terminated(void) {
	struct string_buffer sb = string_buffer_new(NULL);
	string_buffer_append(&sb, "Hello world");
	const char *hello_world = string_buffer_terminate(&sb);
	utest_assert_true(strcmp(hello_world, "Hello world") == 0);
	string_buffer_append(&sb, "...");
	const char *hello_world_dots = string_buffer_terminate(&sb);
	utest_assert_true(strcmp(hello_world_dots, "Hello world...") == 0);
	string_buffer_free(&sb);
}

static void test_sb_append_format_terminated(void) {
	// Check for broken snprintf implementation
	struct string_buffer sb = string_buffer_new("Hdkajdshfksajdlfsahklfhsldldfsad");
	string_buffer_clear(&sb);
	sb.buf_cap = 0;

	string_buffer_clear(&sb);
	string_buffer_append_format(&sb, "%s", "Hello world");
	const char *hello_world = string_buffer_terminate(&sb);
	utest_assert_true(strcmp(hello_world, "Hello world") == 0);
	string_buffer_free(&sb);
}

int main(void) {
	utest_run(test_sb_new_unterminated,"Hello world");
	utest_run(test_sb_new_terminated,"Hello world");
	utest_run(test_sb_append_unterminated,);
	utest_run(test_sb_append_terminated,);
	utest_run(test_sb_append_format_terminated,);

	return utest_end();
}
