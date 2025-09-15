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
#include <stdio.h>
#include <stdarg.h>

#include "string_buffer.h"
#include "alloc.h"

bool string_buffer_ensure_len(struct string_buffer *sb, size_t needed) {
	return util_alloc_ensure_len(sb->buf, needed, sb->buf_cap);
}

int string_buffer_append_format(struct string_buffer *sb, const char *fmt, ...) {
	int res;
	va_list args;
	va_start(args, fmt);
	res = string_buffer_append_vformat(sb, fmt, args);
	va_end(args);
	return res;
}

int string_buffer_append_vformat(struct string_buffer *sb, const char *fmt, va_list args) {
	/* We're looping two times to avoid duplicating code */
	for (size_t i = 0; i < 2; i++) {
		va_list arg_copy;
		va_copy(arg_copy, args);
		size_t space_left = sb->buf_cap - sb->buf_len;

		long len = vsnprintf(&sb->buf[sb->buf_len],
                                   space_left, fmt, arg_copy);
		va_end(arg_copy);

		/* Error in vsnprintf() or measured len overflows size_t */
		if (len < 0 || sb->buf_len + len + 1 < sb->buf_len)
			return 0;

		/* There was enough space for the string; we're done */
		if ((size_t)len < space_left) {
			sb->buf_len += len;
			return len;
		}

		/* Not enough space, resize and retry */
		string_buffer_ensure_len(sb, sb->buf_len + len + 1);
	}

	return -1;
}
