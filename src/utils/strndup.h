#ifndef PICCO_SRC_UTIL_STRNDUP_H_
#define PICCO_SRC_UTIL_STRNDUP_H_

#include <stddef.h>
#include <string.h>

#ifndef HAVE_STRNDUP
char *_picco_strndup(const char *s, size_t n);
#define strndup _picco_strndup
#endif

#endif
