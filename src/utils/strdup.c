#include <string.h>
#include <stdlib.h>

#ifndef HAVE_STRDUP
char *_picco_strdup(const char *s) {
	size_t len = strlen(s);
	char *new = (char *)malloc(len + 1);

	if(new == NULL) {
		return NULL;
	}

	new[len] = '\0';
	return (char *)memcpy(new, s, len);
}
#endif
