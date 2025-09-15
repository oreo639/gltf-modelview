#pragma once
#include <stddef.h>
#include <stdbool.h>

struct shader;
struct animation;

struct texture2d {
	unsigned id;
};

struct material {
	// TODO: determine whether to use texture or rgb
	struct texture2d base_color_tex;
	float base_color[4];
};

#define MAX_BONE_INFLUENCE 4

struct vertex {
	float position[3];
	float normal[3];
	float color[4];
	float uv[2];

	short joint_ids[MAX_BONE_INFLUENCE];
	float weights[MAX_BONE_INFLUENCE];
};

struct mesh {
	struct vertex *vertices;
	size_t vertex_count;

	unsigned int *indices;
	size_t index_count;

	struct material *material;

	unsigned int VAO, VBO, EBO;
};

#include <cglm/struct.h>

struct assimp_node_data {
	char* name;
	struct assimp_node_data *children;
	size_t children_count;
	mat4s transformation;
};

struct bone_info {
	size_t id;
	const char *name;
	mat4s inv_bind_matrix;
	mat4s bind_pose;
	mat4s local_transform;
};

struct meshgroup {
	struct mesh *meshes;
	size_t meshes_count;
	size_t *bi;
	size_t bi_count;
};

struct model {
	struct meshgroup *meshgrps;
	size_t meshgrps_count;

	struct bone_info *bone_infos;
	size_t bone_infos_count;
	struct hash_table *bone_info_map;
	struct assimp_node_data root_node;

	struct material *materials;
	size_t materials_count;

	struct animation *animations;
	size_t animations_count;
};

struct animator;

bool model_load_gltf(struct model *model, const char *fpath);
void model_draw(struct model *model, struct animator *anim_state, struct shader *shader);
void model_cleanup(struct model *model);
