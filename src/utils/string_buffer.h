/*
** Copyright (c) 2025 oreo639
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
*/
#ifndef PICCO_SRC_UTIL_STRING_BUFFER_H_
#define PICCO_SRC_UTIL_STRING_BUFFER_H_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

struct string_buffer {
	char *buf;
	size_t buf_len;
	size_t buf_cap;
};

#define string_buffer_ensure_len _picco_string_buffer_ensure_len
#define string_buffer_append_format _picco_string_buffer_append_format
#define string_buffer_append_vformat _picco_string_buffer_append_vformat

bool string_buffer_ensure_len(struct string_buffer *sb, size_t needed);
int string_buffer_append_format(struct string_buffer *sb, const char *fmt, ...);
int string_buffer_append_vformat(struct string_buffer *sb, const char *fmt, va_list args);

static inline int string_buffer_append_len(struct string_buffer* sb, const char *str, size_t slen) {
	if (!string_buffer_ensure_len(sb, sb->buf_len+slen))
		return -1;
	memcpy(&sb->buf[sb->buf_len], str, slen);
	sb->buf_len += slen;
	return slen;
}

static inline int string_buffer_append(struct string_buffer *sb, const char *str) {
	return str ? string_buffer_append_len(sb, str, strlen(str)) : 0;
}

static inline int string_buffer_concat(struct string_buffer *sb, const struct string_buffer* sb2) {
	return string_buffer_append_len(sb, sb2->buf, sb2->buf_len);
}

static inline void string_buffer_clear(struct string_buffer *sb) {
	sb->buf_len = 0;
}

static inline char *string_buffer_terminate(struct string_buffer *sb) {
	string_buffer_ensure_len(sb, sb->buf_len+1);
	sb->buf[sb->buf_len] = '\0';
	return sb->buf;
}

static inline struct string_buffer string_buffer_new(const char *str) {
	struct string_buffer sb = {0};
	string_buffer_ensure_len(&sb, 1);
	string_buffer_append(&sb, str);
	return sb;
}

static inline void string_buffer_free(struct string_buffer *sb) {
	free(sb->buf);
	sb->buf = NULL;
	sb->buf_cap = 0;
}

#endif
