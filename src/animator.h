#include <cglm/struct.h>

struct animator {
	mat4s *final_bone_matrices;
	size_t final_bone_matrices_count;
	const struct model *model;
	const struct animation *current_animation;
	float current_time;
	float delta_time;
};

bool animator_init(struct animator *animator, const struct animation *anim, const struct model *model);
bool animator_change_animation(struct animator *animator, const struct animation *anim);
void animator_cleanup(struct animator *animator);
void animator_update(struct animator* animator, float dt);
