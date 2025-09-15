#pragma once
#include <stdbool.h>

struct shader {
	unsigned id;
};

bool shader_create(struct shader *shader, const char* vertexPath, const char* fragmentPath);
void shader_cleanup(struct shader *shader);
bool shader_use(struct shader *shader);
