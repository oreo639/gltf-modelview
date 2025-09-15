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
#ifndef PICCO_SRC_UTIL_ALLOC_H_
#define PICCO_SRC_UTIL_ALLOC_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include <stdbit.h>

#define util_alloc_ensure_len(data, len, capacity) \
	_picco_util_alloc_ensure_len((char**)&(data), sizeof(*(data)), (len), &(capacity))

static inline bool _picco_util_alloc_ensure_len(char **data, size_t size, size_t count, size_t *capacity) {
	if (count >= *capacity) {
		void *tmp;
		size_t n = stdc_bit_ceil(count);
		if (!n) return false;
		tmp = realloc(*data, n * size);
		if (tmp == NULL) return false;
		*data = tmp;
		*capacity = n;
	}
	return true;
}
#endif
