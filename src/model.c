#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <stdbool.h>
#include <assert.h>

#include <epoxy/gl.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <cglm/cglm.h>
#include <cglm/struct.h>
#include "utils/string_buffer.h"
#include "utils/strdup.h"
#include "utils/hash_table.h"
#include "utils/darray.h"

#include "model.h"
#include "animation.h"
#include "animator.h"
#include "shader.h"
#include "image.h"

#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define LOG(...) (fprintf(stderr, __VA_ARGS__),fprintf(stderr, "\n"))
static const char *str_cgltf_accessor_comp_type(enum cgltf_component_type component_type);

static void gl_setup_mesh(struct mesh *mesh);
static void gl_draw_mesh(struct mesh *mesh, struct shader *shader);
static void gl_setup_texture(struct texture2d *tex, struct image_tex *image);

static struct material *mdl_get_default_material(void) {
	static struct material *default_mat = NULL;
	if (default_mat == NULL) {
		default_mat = calloc(1, sizeof(struct material));

		default_mat->base_color[0] = 1.0f;
		default_mat->base_color[1] = 0.0f;
		default_mat->base_color[2] = 1.0f;
		default_mat->base_color[3] = 1.0f;

		struct image_tex image = {0};
		image_load_file(&image, "resources/uv.png");
		gl_setup_texture(&default_mat->base_color_tex, &image);
		image_free(&image);
	}

	return default_mat;
}

static void load_image_from_cgltf_image(struct image_tex *image, cgltf_image *image_cgltf, const char *gltf_path) {
	if (!image_cgltf) return;

	if (image_cgltf->uri) {
		if (strncmp(image_cgltf->uri, "data:", 5) == 0) {
			// Load cgltf_image as base64
			// Data URI Format: data:<mediatype>;base64,<data>
			LOG("IMAGE: glTF base64 embeded is highly discouraged");
			int i = 0;
			while ((image_cgltf->uri[i] != ',') && (image_cgltf->uri[i] != 0)) i++;

			if (image_cgltf->uri[i] == 0) {
				LOG("IMAGE: glTF data URI is not a valid image");
			} else {
				int base64Size = (int)strlen(image_cgltf->uri + i + 1);
				while (image_cgltf->uri[i + base64Size] == '=') base64Size--;	// Ignore optional paddings
				int numberOfEncodedBits = base64Size*6 - (base64Size*6) % 8 ;   // Encoded bits minus extra bits, so it becomes a multiple of 8 bits
				int outSize = numberOfEncodedBits/8 ;                           // Actual encoded bytes
				void *data = NULL;

				cgltf_options options = {0};
				cgltf_result result = cgltf_load_buffer_base64(&options, outSize, image_cgltf->uri + i + 1, &data);

				if (result == cgltf_result_success) {
					image_load_mem(image, data, outSize);
					free(data);
				}
			}
		} else {
			char* path = calloc(strlen(image_cgltf->uri) + strlen(gltf_path) + 1, sizeof(char));
			cgltf_combine_paths(path, gltf_path, image_cgltf->uri);
			cgltf_decode_uri(path + strlen(path) - strlen(image_cgltf->uri));

			printf("%s\n", path);
			image_load_file(image, path);
			free(path);
		}
	} else if (image_cgltf->buffer_view && image_cgltf->buffer_view->buffer->data) {
		unsigned char *data = malloc(image_cgltf->buffer_view->size);
		size_t offset = image_cgltf->buffer_view->offset;
		size_t stride = image_cgltf->buffer_view->stride ? image_cgltf->buffer_view->stride : 1;
		for (size_t i = 0; i < image_cgltf->buffer_view->size; i++) {
			data[i] = ((unsigned char*)image_cgltf->buffer_view->buffer->data)[offset];
			offset += stride;
		}

		// Check mime_type for image: (cgltfImage->mime_type == "image/png")
		// NOTE: Detected that some models define mime_type as "image\\/png"
		if ((strcmp(image_cgltf->mime_type, "image\\/png") == 0) || (strcmp(image_cgltf->mime_type, "image/png") == 0))
			image_load_mem(image, data, image_cgltf->buffer_view->size);
		else if ((strcmp(image_cgltf->mime_type, "image\\/jpeg") == 0) || (strcmp(image_cgltf->mime_type, "image/jpeg") == 0))
			image_load_mem(image, data, image_cgltf->buffer_view->size);
		else LOG("MODEL: glTF image data MIME type not recognized: %s", image_cgltf->mime_type);

		free(data);
	}
}

#define get_attribute(_name) \
	((acc = hash_table_get(&ht, _name)) && \
	 (acc->count == mdl_mesh->vertex_count || (DEBUG("WARNING: attribute '%s' has mismatched attributed count, ignoring...\n", _name),0)) \
	)

static bool model_load_gltf_mesh(const struct model *model, struct meshgroup *meshgrp, const cgltf_mesh *gltf_mesh, const cgltf_data *data) {
	meshgrp->meshes_count = gltf_mesh->primitives_count;
	meshgrp->meshes = calloc(meshgrp->meshes_count, sizeof(struct mesh));

	for (size_t pi = 0; pi < gltf_mesh->primitives_count; pi++) {
		struct mesh *mdl_mesh = &meshgrp->meshes[pi];
		const cgltf_primitive *prim = &gltf_mesh->primitives[pi];

		struct hash_table ht = {0};
		hash_table_init(&ht, 2, hash_fnv1a32_str, hash_key_equals_str, NULL, NULL);
		for (size_t ai = 0; ai < prim->attributes_count; ++ai) {
			const cgltf_attribute *attrib = &prim->attributes[ai];
			const cgltf_accessor *acc = attrib->data;
			DEBUG("mesh primitive %ld) `%s` (comp type: %ldx%s)\n", pi, attrib->name, cgltf_num_components(acc->type), str_cgltf_accessor_comp_type(acc->component_type));
			DEBUG("primtype: %d\n", prim->type);
			hash_table_insert(&ht, attrib->name, (void*)acc, true);
		}

		const cgltf_accessor *acc = NULL;
		if ((acc = hash_table_get(&ht, "POSITION"))) {
			mdl_mesh->vertex_count = acc->count;
			mdl_mesh->vertices = calloc(mdl_mesh->vertex_count, sizeof(struct vertex));
			for (size_t i = 0; i < mdl_mesh->vertex_count; i++)
				cgltf_accessor_read_float(acc, i, mdl_mesh->vertices[i].position, 3);
		} else {
			DEBUG("mesh does not contain valid position data, skipping\n");
			continue;
		}

		if (get_attribute("COLOR_0")) {
			size_t num = cgltf_num_components(acc->type);
			for (size_t i = 0; i < acc->count; i++) {
				cgltf_accessor_read_float(acc, i, mdl_mesh->vertices[i].color, num);
				for (size_t c = num; c < 4; ++c) {
					mdl_mesh->vertices[i].color[c] = 1.0;
				}
			}
		} else {
			for (size_t a = 0; a < mdl_mesh->vertex_count; a++) {
				float *colr = mdl_mesh->vertices[a].color;
				colr[0] = 1.0;
				colr[1] = 1.0;
				colr[2] = 1.0;
				colr[3] = 1.0;
			}
		}

		if (get_attribute("NORMAL")) {
			for (size_t i = 0; i < acc->count; i++)
				cgltf_accessor_read_float(acc, i, mdl_mesh->vertices[i].normal, 3);
		}

		//if ((acc = hash_table_get(&ht, "TANGENT")) && acc->count == mdl_mesh->vertex_count) {
		//	for (size_t i = 0; i < acc->count; i++)
		//		cgltf_accessor_read_float(acc, i, mdl_mesh->vertices[i].tangent, 3);
		//}

		if (get_attribute("TEXCOORD_0")) {
			for (size_t i = 0; i < acc->count; i++)
				cgltf_accessor_read_float(acc, i, mdl_mesh->vertices[i].uv, 2);
		}

		if (get_attribute("JOINTS_0")) {
			for (size_t i = 0; i < acc->count; i++) {
				cgltf_uint joint_ids[4] = {0};
				cgltf_accessor_read_uint(acc, i, joint_ids, 4);
				for (size_t j = 0; j < MAX_BONE_INFLUENCE; ++j) {
					//if (joint_ids[j] != (unsigned char)joint_ids[j]) LOG("Specified bone ID (%u) is too large, clamping value\n", joint_ids[j]);
					mdl_mesh->vertices[i].joint_ids[j] = joint_ids[j];
				}
			}
		}

		if (get_attribute("WEIGHTS_0")) {
			for (size_t i = 0; i < acc->count; i++)
				cgltf_accessor_read_float(acc, i, mdl_mesh->vertices[i].weights, MAX_BONE_INFLUENCE);
		}

		hash_table_fini(&ht);

		if (gltf_mesh->primitives[pi].indices) {
			cgltf_accessor *prim_acc = gltf_mesh->primitives[pi].indices;
			mdl_mesh->indices = malloc(prim_acc->count*sizeof(unsigned int));
			for (unsigned int a = 0; a < prim_acc->count; a++) {
				mdl_mesh->indices[a] = cgltf_accessor_read_index(prim_acc, a);
			}
			mdl_mesh->index_count = prim_acc->count;
		}

		if (gltf_mesh->primitives[pi].material) {
			size_t mati = gltf_mesh->primitives[pi].material-data->materials;
			mdl_mesh->material = &model->materials[mati];
		} else {
			mdl_mesh->material = mdl_get_default_material();
		}
	}
	return true;
}

void gltf_read_hierarchy_data(struct assimp_node_data *dest, const cgltf_node* src) {
	dest->name = src->name ? strdup(src->name) : strdup("##generic_node");
	cgltf_float localTransform[16];
	cgltf_node_transform_local(src, localTransform);
	dest->transformation = glms_mat4_make(localTransform);
	dest->children_count = src->children_count;

	dest->children = calloc(src->children_count, sizeof(struct assimp_node_data));
	for (size_t i = 0; i < src->children_count; ++i)
	{
		gltf_read_hierarchy_data(&dest->children[i], src->children[i]);
	}
}

bool model_load_gltf(struct model *model, const char *fpath) {
	cgltf_options options = {0};
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, fpath, &data);
	if (result != cgltf_result_success) {
		fprintf(stderr, "Failed to load model: %s\n", fpath);
		return false;
	}

	DEBUG("MODEL: [%s] glTF meshes (%s) count: %li\n", fpath, (data->file_type == 2) ? "glb" : "gltf", data->meshes_count);
	DEBUG("MODEL: [%s] glTF materials (%s) count: %li\n", fpath, (data->file_type == 2) ? "glb" : "gltf", data->materials_count);

	result = cgltf_load_buffers(&options, data, fpath);
	if (result != cgltf_result_success) {
		fprintf(stderr, "Model load failed [%s]: Failed to load mesh/material buffers\n", fpath);
		cgltf_free(data);
		return false;
	}

	model->materials = calloc(data->materials_count, sizeof(struct material));
	model->materials_count = data->materials_count;
	for (size_t mi = 0; mi < data->materials_count; mi++) {
		const cgltf_material *material = &data->materials[mi];

		model->materials[mi].base_color[0] = material->pbr_metallic_roughness.base_color_factor[0];
		model->materials[mi].base_color[1] = material->pbr_metallic_roughness.base_color_factor[1];
		model->materials[mi].base_color[2] = material->pbr_metallic_roughness.base_color_factor[2];
		model->materials[mi].base_color[3] = material->pbr_metallic_roughness.base_color_factor[3];
		if (material->pbr_metallic_roughness.base_color_texture.texture) {
			cgltf_texture *tex = material->pbr_metallic_roughness.base_color_texture.texture;
			struct image_tex image = {0};
			load_image_from_cgltf_image(&image, tex->image, fpath);
			gl_setup_texture(&model->materials[mi].base_color_tex, &image);
			image_free(&image);
		} else {
			model->materials[mi].base_color_tex = mdl_get_default_material()->base_color_tex;
		}
	}

	if (data->scenes_count <= 0) {
		cgltf_free(data);
		return true;
	}

	cgltf_scene *scene = data->scene ? data->scene : &data->scenes[0];

	size_t mesh_nodes_count = 0;
	for (size_t ni = 0; ni < data->nodes_count; ++ni)
		if (data->nodes[ni].mesh) ++mesh_nodes_count;

	model->meshgrps_count = mesh_nodes_count;
	model->meshgrps = calloc(model->meshgrps_count, sizeof(struct meshgroup));
	struct hash_table *bone_info_map = hash_table_create(2, hash_fnv1a32_str, hash_key_equals_str, NULL, NULL);
	darray(struct bone_info) bone_infos = {0};
	for (size_t ni = 0, mi = 0; ni < data->nodes_count; ++ni) {
		const cgltf_node *node = &data->nodes[ni];
		if (!node->mesh) continue;
		const cgltf_mesh *mesh = node->mesh;
		const cgltf_skin *skin = node->skin;
		struct meshgroup *meshgrp = &model->meshgrps[mi++];
		if (mesh->name) DEBUG("mesh %zu name: `%s` (%zu)\n", mi, mesh->name, mesh->primitives_count);
		model_load_gltf_mesh(model, meshgrp, mesh, data);
		DEBUG("---\n");
		if (skin) {
			meshgrp->bi_count = skin->joints_count;
			meshgrp->bi = calloc(meshgrp->bi_count, sizeof(struct bone_info*));
			for (size_t ji = 0; ji < skin->joints_count; ji++) {
				cgltf_node *joint = skin->joints[ji];
				if (!joint->name) {
					struct string_buffer sb = string_buffer_new("bone.");
					string_buffer_append_format(&sb, "%zu", joint-data->nodes);
					joint->name = string_buffer_terminate(&sb);
				}
				struct bone_info *bone_info = NULL;
				size_t bi = 0;
				if (!hash_table_lookup(bone_info_map, joint->name, (void**)&bi)) {
					darray_push_back(&bone_infos, (struct bone_info){.name = strdup(joint->name), .id = bone_infos.len});
					bone_info = &bone_infos.data[bone_infos.len-1];
					//DEBUG("bone: %s\n", bone_info->name);
					hash_table_insert(bone_info_map, (char*)bone_info->name, (void*)bone_info->id, true);
				} else {
					bone_info = &bone_infos.data[bi];
				}
				meshgrp->bi[ji] = bone_info->id;
				cgltf_float worldTransform[16];
				cgltf_float localTransform[16];
				cgltf_float invBind[16];
				cgltf_node_transform_world(joint, worldTransform);
				cgltf_node_transform_local(joint, localTransform);
				cgltf_accessor_read_float(skin->inverse_bind_matrices, ji, invBind, 16);
				bone_info->bind_pose = glms_mat4_make(worldTransform);
				bone_info->inv_bind_matrix = glms_mat4_make(invBind);
				bone_info->local_transform = glms_mat4_make(localTransform);
			}
		}
	}
	model->bone_infos = bone_infos.data;
	model->bone_infos_count = bone_infos.len;
	model->bone_info_map = bone_info_map;
	if (model->bone_infos_count != 0) {
		if (scene->nodes_count == 1) {
			gltf_read_hierarchy_data(&model->root_node, scene->nodes[0]);
		} else if (scene->nodes_count > 1) {
			model->root_node.name = strdup("##root");
			model->root_node.transformation = GLMS_MAT4_IDENTITY;
			model->root_node.children_count = scene->nodes_count;
			model->root_node.children = calloc(scene->nodes_count, sizeof(struct assimp_node_data));
			for (size_t sn = 0; sn < scene->nodes_count; ++sn)
				gltf_read_hierarchy_data(&model->root_node.children[sn], scene->nodes[sn]);
		}
	}

	model->animations_count = data->animations_count;
	model->animations = calloc(model->animations_count, sizeof(struct animation));
	for (size_t ai = 0; ai < data->animations_count; ai++) {
		const cgltf_animation *anim = &data->animations[ai];
		if (anim->name) {
			model->animations[ai].name = strdup(anim->name);
		} else {
			struct string_buffer sb = string_buffer_new("sequence");
			string_buffer_append_format(&sb, "%zu", ai);
			model->animations[ai].name = string_buffer_terminate(&sb);
		}
		DEBUG("Animation name: '%s'\n", model->animations[ai].name);

		model->animations[ai].ticks_per_second = 1.0f;
		//model->animations[ai].ticks_per_second = 1000.0f;
		struct hash_table *ht = hash_table_create(2, hash_fnv1a32_str, hash_key_equals_str, NULL, NULL);
		darray(struct bone) bones = {0};
		float duration = 0;
		for (size_t ci = 0; ci < anim->channels_count; ++ci) {
			const cgltf_animation_channel *channel = &anim->channels[ci];
			const cgltf_animation_sampler *sampler = channel->sampler;
			const char *boneName = channel->target_node->name;
			if (!channel->target_node->name) {
				struct string_buffer sb = string_buffer_new("bone.");
				string_buffer_append_format(&sb, "%zu", channel->target_node-data->nodes);
				channel->target_node->name = string_buffer_terminate(&sb);
				boneName = channel->target_node->name;
			}
			struct bone *bone = NULL;
			size_t bi = 0;
			if (!hash_table_lookup(ht, boneName, (void**)&bi)) {
				darray_push_back(&bones, (struct bone){.name = strdup(boneName)});
				bone = &bones.data[bones.len-1];
				hash_table_insert(ht, (char*)bone->name, (void*)bones.len-1, true);
			} else {
				bone = &bones.data[bi];
			}
			//fprintf(stderr, "%p\n", bone);
			//const char *map[] = {
			//	[cgltf_animation_path_type_rotation] = "rotation",
			//	[cgltf_animation_path_type_scale] = "scale",
			//	[cgltf_animation_path_type_translation] = "translation",
			//};
			//fprintf(stderr, "Name: %s:%s, %s\n", boneName, bone->name, map[channel->target_path]);
			switch (channel->target_path) {
			case cgltf_animation_path_type_rotation:
				assert(bone->rotations == NULL);
				bone->num_rotations = sampler->input->count;
				bone->rotations = calloc(bone->num_rotations, sizeof(struct key_rotation));
				for (size_t i = 0; i < bone->num_rotations; ++i) {
					cgltf_float time = 0;
					cgltf_float rotation[4];
					cgltf_accessor_read_float(sampler->input, i, &time, 1);
					cgltf_accessor_read_float(sampler->output, i, rotation, 4);
					bone->rotations[i].time_stamp = time;
					bone->rotations[i].orientation = glms_quat_make(rotation);
				}
				if (bone->rotations[bone->num_rotations-1].time_stamp > duration)
					duration = bone->rotations[bone->num_rotations-1].time_stamp;
				break;
			case cgltf_animation_path_type_scale:
				assert(bone->scales == NULL);
				bone->num_scales = sampler->input->count;
				bone->scales = calloc(bone->num_scales, sizeof(struct key_scale));
				for (size_t i = 0; i < bone->num_scales; ++i) {
					cgltf_float time = 0;
					cgltf_float scale[3];
					cgltf_accessor_read_float(sampler->input, i, &time, 1);
					cgltf_accessor_read_float(sampler->output, i, scale, 3);
					bone->scales[i].time_stamp = time;
					bone->scales[i].scale = glms_vec3_make(scale);
				}
				if (bone->scales[bone->num_scales-1].time_stamp > duration)
					duration = bone->scales[bone->num_scales-1].time_stamp;
				break;
			case cgltf_animation_path_type_translation:
				assert(bone->positions == NULL);
				bone->num_positions = sampler->input->count;
				bone->positions = calloc(bone->num_positions, sizeof(struct key_position));
				for (size_t i = 0; i < bone->num_positions; ++i) {
					assert(cgltf_num_components(sampler->output->type) >= 3);
					cgltf_float time = 0;
					cgltf_float translation[3];
					cgltf_accessor_read_float(sampler->input, i, &time, 1);
					cgltf_accessor_read_float(sampler->output, i, translation, 3);
					bone->positions[i].time_stamp = time;
					bone->positions[i].position = glms_vec3_make(translation);
				}
				if (bone->positions[bone->num_positions-1].time_stamp > duration)
					duration = bone->positions[bone->num_positions-1].time_stamp;
				break;
			default:
				fprintf(stderr, "Model animation contains unhandled target_path on channel %zu for animation %s, it will be skipped\n", ci, model->animations[ai].name);
				break;
			}
		}
		hash_table_destroy(ht);
		model->animations[ai].bones = bones.data;
		model->animations[ai].bones_count = bones.len;
		model->animations[ai].duration = duration;
		DEBUG("Bones: %zu\nMax duration: %f\n", model->animations[ai].bones_count, model->animations[ai].duration);
	}

	for (size_t mi = 0; mi < model->meshgrps_count; ++mi)
		for (size_t pi = 0; pi < model->meshgrps[mi].meshes_count; ++pi)
			gl_setup_mesh(&model->meshgrps[mi].meshes[pi]);

	cgltf_free(data);
	return true;
}

static void meshgrp_draw(struct meshgroup *meshgrp, struct bone_info *bone_infos, struct animator *animator, struct shader *shader) {
	glUniform1i(glGetUniformLocation(shader->id, "bUseBoneMatrices"), !!meshgrp->bi_count);
	for (size_t i = 0; i < meshgrp->bi_count; i++) {
		struct bone_info *bone_info = &bone_infos[meshgrp->bi[i]];
		struct string_buffer sb = string_buffer_new("finalBonesMatrices");
		string_buffer_append_format(&sb, "[%d]", i);
		mat4s finalBoneMtx = glms_mat4_mul(bone_info->bind_pose, bone_info->inv_bind_matrix);
		if (animator)
			finalBoneMtx = animator->final_bone_matrices[bone_info->id];
		glUniformMatrix4fv(glGetUniformLocation(shader->id, string_buffer_terminate(&sb)), 1, GL_FALSE, &finalBoneMtx.raw[0][0]);
		string_buffer_free(&sb);
	}

	for (size_t pi = 0; pi < meshgrp->meshes_count; ++pi)
		gl_draw_mesh(&meshgrp->meshes[pi], shader);
}

void model_draw(struct model *model, struct animator *animator, struct shader *shader) {
	shader_use(shader);

	for (size_t mi = 0; mi < model->meshgrps_count; ++mi)
		meshgrp_draw(&model->meshgrps[mi], model->bone_infos, animator, shader);
}

void cleanup_node(struct assimp_node_data *node) {
	for (size_t ni = 0; ni < node->children_count; ++ni) {
		cleanup_node(&node->children[ni]);
	}
	free(node->name);
	free(node->children);
}

void model_cleanup(struct model *model) {
	for (size_t mi = 0; mi < model->meshgrps_count; mi++) {
		for (size_t pi = 0; pi < model->meshgrps[mi].meshes_count; ++pi) {
			free(model->meshgrps[mi].meshes[pi].vertices);
			free(model->meshgrps[mi].meshes[pi].indices);
		}
		free(model->meshgrps[mi].meshes);
		free(model->meshgrps[mi].bi);
	}
	free(model->meshgrps);
	for (size_t bi = 0; bi < model->bone_infos_count; ++bi) {
		free((char*)model->bone_infos[bi].name);
	}
	free(model->bone_infos);
	hash_table_destroy(model->bone_info_map);
	cleanup_node(&model->root_node);
	for (size_t ai = 0; ai < model->animations_count; ++ai) {
		free(model->animations[ai].name);
		for (size_t bi = 0; bi < model->animations[ai].bones_count; ++bi) {
			free((char*)model->animations[ai].bones[bi].name);
			free(model->animations[ai].bones[bi].positions);
			free(model->animations[ai].bones[bi].rotations);
			free(model->animations[ai].bones[bi].scales);
		}
		free(model->animations[ai].bones);
	}
	free(model->animations);
	free(model->materials);
}

static const char *str_cgltf_accessor_comp_type(enum cgltf_component_type component_type) {
	switch (component_type) {
	case cgltf_component_type_r_8:      return "i8";
	case cgltf_component_type_r_8u:     return "u8";
	case cgltf_component_type_r_16:     return "i16";
	case cgltf_component_type_r_16u:    return "u16";
	case cgltf_component_type_r_32u:    return "u32";
	case cgltf_component_type_r_32f:    return "f32";
	case cgltf_component_type_invalid:  return "inv";
	case cgltf_component_type_max_enum: return "err";
	}
	return "unk";
}

static void gl_setup_mesh(struct mesh *mesh) {
	glGenVertexArrays(1, &mesh->VAO);
	glGenBuffers(1, &mesh->VBO);
	glGenBuffers(1, &mesh->EBO);

	glBindVertexArray(mesh->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
	glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count*sizeof(struct vertex), mesh->vertices, GL_STATIC_DRAW);

	// position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*)offsetof(struct vertex, position));

	// color attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*)offsetof(struct vertex, color));

	// normal attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*)offsetof(struct vertex, normal));

	// Texcoords attribute
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*)offsetof(struct vertex, uv));

	// bones
	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 4, GL_SHORT, sizeof(struct vertex), (void*)offsetof(struct vertex, joint_ids));

	// weights
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*)offsetof(struct vertex, weights));

	if (mesh->indices) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count*sizeof(unsigned int), mesh->indices, GL_STATIC_DRAW);
	}
}

static void gl_draw_mesh(struct mesh *mesh, struct shader *shader) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->material->base_color_tex.id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glUniform4fv(glGetUniformLocation(shader->id, "base_color"), 1, &mesh->material->base_color[0]);

	// draw mesh
	glBindVertexArray(mesh->VAO);
	if (mesh->indices)
		glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	else
		glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count*3);
	glBindVertexArray(0);
}

static void gl_setup_texture(struct texture2d *tex, struct image_tex *image) {
	if (!image->data) return;

	unsigned int textureID;
	glGenTextures(1, &textureID);
	GLenum format = GL_RGB;
	if (image->fmt == image_texture_bw)
		format = GL_RED;
	else if (image->fmt == image_texture_rgb)
		format = GL_RGB;
	else if (image->fmt == image_texture_rgba)
		format = GL_RGBA;

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE, image->data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	tex->id = textureID;
}
