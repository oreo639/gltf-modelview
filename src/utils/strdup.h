#ifndef PICCO_SRC_UTIL_STRDUP_H_
#define PICCO_SRC_UTIL_STRDUP_H_

#include <string.h>

#ifndef HAVE_STRDUP
char *_picco_strdup(const char *s);
#define strdup _picco_strdup
#endif

#endif
