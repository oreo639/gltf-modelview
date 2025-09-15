#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include <cglm/struct.h>

#include "bone.h"

static size_t get_poisiton_index(struct bone *bone, float animation_time) {
	if (bone->num_positions == 1)
		return 0;
	for (size_t i = 0; i < bone->num_positions - 1; ++i) {
		if (animation_time < bone->positions[i + 1].time_stamp) {
			return i;
		}
	}
	assert(false);
	return 0;
}

static size_t get_rotation_index(struct bone *bone, float animation_time) {
	if (bone->num_rotations == 1)
		return 0;
	for (size_t i = 0; i < bone->num_rotations - 1; i++) {
		if (animation_time < bone->rotations[i + 1].time_stamp) {
			return i;
		}
	}
	assert(false);
	return 0;
}

static size_t get_scale_index(struct bone *bone, float animation_time) {
	if (bone->num_scales == 1)
		return 0;
	for (size_t i = 0; i < bone->num_scales - 1; i++) {
		if (animation_time < bone->scales[i + 1].time_stamp) {
			return i;
		}
	}
	assert(false);
	return 0;
}

static float get_scale_factor(float last_time_stamp, float next_time_stamp, float animation_time) {
	float mid_way_length = animation_time - last_time_stamp;
	float frames_diff = next_time_stamp - last_time_stamp;
	return mid_way_length / frames_diff;
}

static vec3s interpolate_position(struct bone *bone, float animation_time) {
	if (bone->num_positions == 1)
		return bone->positions[0].position;
	size_t p0_index = get_poisiton_index(bone, animation_time);
	size_t p1_index = p0_index + 1;
	float scale_factor = get_scale_factor(bone->positions[p0_index].time_stamp, bone->positions[p1_index].time_stamp, animation_time);
	vec3s final_position = glms_vec3_mix(bone->positions[p0_index].position, bone->positions[p1_index].position, scale_factor);
	return final_position;
}

static versors interpolate_rotation(struct bone *bone, float animation_time) {
	if (bone->num_rotations == 1) {
		versors dest = glms_quat_normalize(bone->rotations[0].orientation);
		return dest;
	}
	size_t p0_index = get_rotation_index(bone, animation_time);
	size_t p1_index = p0_index + 1;
	float scale_factor = get_scale_factor(bone->rotations[p0_index].time_stamp, bone->rotations[p1_index].time_stamp, animation_time);

	versors final_rotation = glms_quat_slerp(bone->rotations[p0_index].orientation, bone->rotations[p1_index].orientation, scale_factor);
	final_rotation = glms_quat_normalize(final_rotation);
	return final_rotation;
}

static vec3s interpolate_scaling(struct bone *bone, float animation_time) {
	if (bone->num_scales == 1)
		return bone->scales[0].scale;
	size_t p0_index = get_scale_index(bone, animation_time);
	size_t p1_index = p0_index + 1;
	float scale_factor = get_scale_factor(bone->scales[p0_index].time_stamp, bone->scales[p1_index].time_stamp, animation_time);
	vec3s final_position = glms_vec3_mix(bone->scales[p0_index].scale, bone->scales[p1_index].scale, scale_factor);
	return final_position;
}

void bone_update(struct bone* bone, float animation_time) {
	if (bone->num_positions != 0)
		bone->local_position = interpolate_position(bone, animation_time);
	if (bone->num_rotations != 0)
		bone->local_rotation = interpolate_rotation(bone, animation_time);
	if (bone->num_scales != 0)
		bone->local_scale = interpolate_scaling(bone, animation_time);
}
