#include <string.h>
#include <stdlib.h>

#ifndef HAVE_STRNDUP
#ifndef HAVE_STRNLEN
#define strnlen _picco_strnlen
static size_t _picco_strnlen(const char *s, size_t n)
{
	const char *p = memchr(s, 0, n);
	return p ? (size_t)(p-s) : n;
}
#endif // HAVE_STRNLEN

char *_picco_strndup(const char *s, size_t n)
{
	size_t len = strnlen(s, n);
	char *new = (char *)malloc(len + 1);

	if(new == NULL) {
		return NULL;
	}

	new[len] = '\0';
	return (char *)memcpy(new, s, len);
}
#endif // HAVE_STRNDUP
