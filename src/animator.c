#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <cglm/cglm.h>
#include <cglm/struct.h>

#include "utils/hash_table.h"

#include "animator.h"
#include "animation.h"
#include "model.h"

bool animator_init(struct animator *animator, const struct animation *anim, const struct model *model) {
	animator->model = model;
	animator->current_animation = anim;
	animator->current_time = 0.0f;
	animator->final_bone_matrices = calloc(model->bone_infos_count, sizeof(mat4));
	for (size_t i = 0; i < model->bone_infos_count; ++i) {
		animator->final_bone_matrices[i] = GLMS_MAT4_IDENTITY;
	}
	return true;
}

void animator_cleanup(struct animator *animator) {
	free(animator->final_bone_matrices);
}

bool animator_change_animation(struct animator *animator, const struct animation *anim) {
	animator->current_animation = anim;
	animator->current_time = 0.0f;
	return true;
}

static void calculate_bone_transform(struct animator* animator, const struct assimp_node_data* node, mat4s parent_transform);

void animator_update(struct animator* animator, float dt) {
	//printf("%f\n", animator->current_time);
	animator->delta_time = dt;
	if (animator->current_animation) {
		animator->current_time += animator->current_animation->ticks_per_second * dt;
		animator->current_time = fmod(animator->current_time, animator->current_animation->duration);
		calculate_bone_transform(animator, &animator->model->root_node, GLMS_MAT4_IDENTITY);
	}
}

void animator_play_animation(struct animator* animator, struct animation* animation) {
    animator->current_animation = animation;
    animator->current_time = 0.0f;
}

static struct bone* find_bone(const struct animation* animation, const char* name) {
	for (size_t i = 0; i < animation->bones_count; i++) {
		if (strcmp(animation->bones[i].name, name) == 0) {
			return &animation->bones[i];
		}
	}
	return NULL;
}

static void calculate_bone_transform(struct animator* animator, const struct assimp_node_data* node, mat4s parent_transform) {
	char* node_name = node->name;
	mat4s node_transform = node->transformation;
	struct bone_info *bone_info = NULL;
	size_t bi_idx = 0;
	if (hash_table_lookup(animator->model->bone_info_map, node_name, (void**)&bi_idx)) {
		bone_info = &animator->model->bone_infos[bi_idx];
	}
	struct bone *bone = find_bone(animator->current_animation, node_name);
	if (bone) {
		vec4s dec_position = GLMS_VEC4_ZERO;
		mat4s dec_rotation = GLMS_MAT4_IDENTITY;
		vec3s dec_scale = GLMS_VEC3_ONE;
		if (bone_info) glms_decompose(bone_info->local_transform, &dec_position, &dec_rotation, &dec_scale);
		if (bone->num_positions == 0)
			bone->local_position = glms_vec4_copy3(dec_position);
		if (bone->num_rotations == 0)
			bone->local_rotation = glms_mat4_quat(dec_rotation);
		if (bone->num_scales == 0)
			bone->local_scale = dec_scale;
		bone_update(bone, animator->current_time);
		vec3s position = bone->local_position;
		versors rotation = bone->local_rotation;
		vec3s scale = bone->local_scale;

		mat4s translation = glms_translate(GLMS_MAT4_IDENTITY, position);
		mat4s rotation_mat = glms_quat_mat4(rotation);
		mat4s scale_mat = glms_scale(GLMS_MAT4_IDENTITY, scale);
		node_transform = glms_mat4_mul(translation, glms_mat4_mul(rotation_mat, scale_mat));
	}
	mat4s global_transform = glms_mat4_mul(parent_transform, node_transform);
	if (bone_info) {
		size_t bone_index = bone_info->id;
		mat4s offset = bone_info->inv_bind_matrix;
		animator->final_bone_matrices[bone_index] = glms_mat4_mul(global_transform, offset);
	}
	for(size_t i = 0; i < node->children_count; i++) {
		calculate_bone_transform(animator, &node->children[i], global_transform);
	}
}
