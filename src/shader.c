#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <epoxy/gl.h>

#include "shader.h"

static uint32_t compileShaderMem(GLenum type, const char* source, int size) {
	uint32_t shader = glCreateShader(type);

	if (!shader) {
		printf("glCreateShader() failed\n");
		return 0;
	}

	glShaderSource(shader, 1, &source, &size);
	glCompileShader(shader);

	int result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (!result) {
		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)malloc(length);
		glGetShaderInfoLog(shader, length, NULL, message);

		printf("Failed to compile shader:\n%s\n", message);
		free(message);

		return 0;
	}

	return shader;
}

static uint32_t compileShader(GLenum type, const char* src_path) {
	FILE* f = fopen(src_path, "rb");
	if (!f) {
		printf("Could not open shader: %s\n", src_path);
		return 0;
	}

	fseek(f, 0, SEEK_END);
	size_t shader_size = ftell(f);
	rewind(f);

	char* source = (char*)malloc(shader_size+1);
	fread(source, 1, shader_size, f);
	source[shader_size] = 0;
	fclose(f);

	uint32_t shader = compileShaderMem(type, source, shader_size);
	free(source);

	if (!shader)
		printf("unable to compile shader %s\n", src_path);

	return shader;
}

bool shader_create(struct shader *shader, const char* vertexPath, const char* fragmentPath) {
	int success;
	char msg[512];

	// Load our shaders
	uint32_t vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath);
	uint32_t fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath);

	shader->id = glCreateProgram();
	glAttachShader(shader->id, vertexShader);
	glAttachShader(shader->id, fragmentShader);
	glLinkProgram(shader->id);
	// check for linking errors
	glGetProgramiv(shader->id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader->id, sizeof(msg), NULL, msg);
		printf("Shader linking failed:\n%s\n", msg);
		return false;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return true;
}

void shader_cleanup(struct shader *shader) {
	glDeleteProgram(shader->id);
}

bool shader_use(struct shader *shader) {
	glUseProgram(shader->id);
	return true;
}
