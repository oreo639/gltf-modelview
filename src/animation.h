#pragma once

#include <stddef.h>
#include <cglm/struct.h>

#include "bone.h"

struct animation {
	char *name;
	float duration;
	int ticks_per_second;
	struct bone *bones;
	size_t bones_count;
};
