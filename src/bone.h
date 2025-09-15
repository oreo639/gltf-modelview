#pragma once

#include <stddef.h>
#include <cglm/struct.h>

struct key_position {
	vec3s position;
	float time_stamp;
};

struct key_rotation {
	versors orientation;
	float time_stamp;
};

struct key_scale {
	vec3s scale;
	float time_stamp;
};

struct bone {
	struct key_position* positions;
	struct key_rotation* rotations;
	struct key_scale* scales;
	size_t num_positions;
	size_t num_rotations;
	size_t num_scales;

	vec3s local_position;
	versors local_rotation;
	vec3s local_scale;

	const char* name;
	size_t id;
};

void bone_update(struct bone* bone, float animation_time);
