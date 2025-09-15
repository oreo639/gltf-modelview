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
#ifndef PICCO_SRC_UTIL_DARRAY_H_
#define PICCO_SRC_UTIL_DARRAY_H_

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "macros.h"

#define darray(_T) \
	struct { _T *data; size_t len; size_t capacity; }

#define darray_decl(_T, _Name) \
	struct _Name { _T *data; size_t len; size_t capacity; }

darray_decl(void, darray_void);

#define darray_init(_da) \
	memset((_da), 0, sizeof(*(_da)))

#define darray_free(_da) \
	( ((_da)->data) ? (free((_da)->data),0) : 0, darray_init(_da) )

#define darray_push_back(da, ...) \
	( darray_expand_((struct darray_void*)(da), sizeof(*(da)->data)) ? -1 : ((da)->data[(da)->len] = (__VA_ARGS__),++(da)->len, 0) )

#define darray_pop_back(_da) \
	( (_da)->len ? (_da)->data[--(_da)->len] : (darray_free(_da), (typeof((_da)->data[0])){0}) )

#define darray_forall(_da, callback) \
	for (size_t _index = 0; _index < (_da)->len; ++_index) callback((_da)->data[_index])

#define darray_size(_da) \
	( (_da)->len )

#define darray_capacity(_da) \
	( (_da)->capacity )

#define darray_get(_da, _index) \
	( assert(_index <= (_da)->len), _index < (_da)->len ? (_da)->data[_index] : (typeof((_da)->data[0])){0} )

#define darray_data(_da) \
	( (_da)->data )

/* Dynamic Array implementation */

static inline int darray_expand_(struct darray_void *da, size_t memsz) {
	if (da->len + 1 > da->capacity) {
		void *tmp;
		size_t n = (da->capacity == 0) ? 1 : da->capacity << 1;
		tmp = realloc(da->data, n * memsz);
		if (tmp == NULL) return -1;
		da->data = tmp;
		da->capacity = n;
	}
	return 0;
}

#endif
