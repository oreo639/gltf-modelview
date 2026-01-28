#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <epoxy/gl.h>
#include <cglm/struct.h>

#include "utils/string_buffer.h"
#include "model.h"
#include "animation.h"
#include "animator.h"
#include "shader.h"

const unsigned int WIDTH = 640, HEIGHT = 480;
unsigned int scr_width = WIDTH, scr_height = HEIGHT;

float angleX = 0.0f, angleY = 0.0f;


vec3s lightPos = (vec3s){{1.2f, 1.0f, 2.0f}};
// camera
vec3s cameraPos   = (vec3s){{0.0f, 0.0f,  3.0f}};
vec3s cameraFront = (vec3s){{0.0f, 0.0f, -1.0f}};
vec3s cameraUp    = (vec3s){{0.0f, 1.0f,  0.0f}};
// euler Angles
float yaw = -90.0f;
float pitch = 0;
float zoom =  45.0f;
const float cameraSpeed = 0.05f; // adjust accordingly

bool exit_requested = false;

void FatalError(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	struct string_buffer msg = {0};
	string_buffer_append_vformat(&msg, fmt, args);
	va_end(args);

	fprintf(stderr, "====Fatal Error Occurred====\n");
	fprintf(stderr, "%s\n", string_buffer_terminate(&msg));
	fprintf(stderr, "\nCannot continue, bailing out!\n");
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error", string_buffer_terminate(&msg), NULL);
	exit(255);
}

void mouse_callback(double xrel, double yrel) {
	float xoffset = xrel;
	float yoffset = -yrel;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw   += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	vec3s direction = {{cos(glm_rad(yaw)) * cos(glm_rad(pitch)), sin(glm_rad(pitch)), sin(glm_rad(yaw)) * cos(glm_rad(pitch))}};
	cameraFront = glms_vec3_normalize(direction);
}

void framebuffer_size_callback(int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, width, height);
}

void window_input_grab(SDL_Window* window, bool enable) {
	if (enable) {
		SDL_HideCursor();
		SDL_SetWindowRelativeMouseMode(window, true);
		SDL_SetWindowMouseGrab(window, true);
	} else {
		SDL_ShowCursor();
		SDL_SetWindowRelativeMouseMode(window, false);
		SDL_SetWindowMouseGrab(window, false);
	}
}

double get_time_sec() {
	Uint64 currentTicks = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();

	return (double)currentTicks / (double)frequency;
}

int main(int argc, char *argv[argc-1]) {
	if (!SDL_Init(0)) {
		FatalError("Failed to init SDL2: %s", SDL_GetError());
	}

	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		FatalError("SDL2 failed to init video: %s", SDL_GetError());
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = NULL;
	SDL_GLContext context = NULL;
	window = SDL_CreateWindow("My Game", WIDTH, HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (window == NULL) {
		FatalError("SDL2 failed to create window: %s", SDL_GetError());
	}

	context = SDL_GL_CreateContext(window);
	if (context == NULL) {
		FatalError("SDL OpenGL creation failed: %s", SDL_GetError());
	}

	printf("GL Vendor: %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer: %s\n", glGetString(GL_RENDERER));
	printf("GL Version: %s\n", glGetString(GL_VERSION));

	struct model fox = {0};
	struct shader fshader[2][2] = {0};
	model_load_gltf(&fox, argc > 1 ? argv[1] : "assets/CesiumMan.gltf");
	shader_create(&fshader[0][0], "shaders/unlit_common.vert.glsl", "shaders/unlit_texture.frag.glsl");
	shader_create(&fshader[0][1], "shaders/unlit_common.vert.glsl", "shaders/unlit_color.frag.glsl");
	shader_create(&fshader[1][0], "shaders/lit_common.vert.glsl", "shaders/lit_texture.frag.glsl");
	shader_create(&fshader[1][1], "shaders/lit_common.vert.glsl", "shaders/lit_color.frag.glsl");

	struct animator anim = {0};
	animator_init(&anim, fox.animations_count > 0 ? &fox.animations[0] : NULL, &fox);

	double last_time = get_time_sec();
	double model_scale = 1.0f;
	bool is_shader_lit = false;
	bool is_shader_color = false;
	bool is_grabbed = true;
	window_input_grab(window, is_grabbed);
	while (!exit_requested) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_EVENT_QUIT:
				exit_requested = true;
				break;
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				scr_width = event.window.data1;
				scr_height = event.window.data2;
				framebuffer_size_callback(event.window.data1, event.window.data2);
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				model_scale += event.wheel.y * .02f;
				break;
			case SDL_EVENT_MOUSE_MOTION:
				if (is_grabbed) mouse_callback(event.motion.xrel, event.motion.yrel);
				break;
			case SDL_EVENT_KEY_UP:
				if (event.key.key == SDLK_ESCAPE) {
					exit_requested = true;
				}
				break;
			case SDL_EVENT_KEY_DOWN:
				if (event.key.key == SDLK_UP) {
					angleX += glm_rad(1);
				} else if (event.key.key == SDLK_DOWN) {
					angleX -= glm_rad(1);
				} else if (event.key.key == SDLK_RIGHT) {
					angleY +=  glm_rad(1);
				} else if (event.key.key == SDLK_LEFT) {
					angleY -= glm_rad(1);
				} else if (event.key.key == SDLK_0) {
					animator_change_animation(&anim, fox.animations_count < 1 ? NULL : &fox.animations[0]);
				} else if (event.key.key == SDLK_1) {
					animator_change_animation(&anim, fox.animations_count < 2 ? NULL : &fox.animations[1]);
				} else if (event.key.key == SDLK_2) {
					animator_change_animation(&anim, fox.animations_count < 3 ? NULL : &fox.animations[2]);
				} else if (event.key.key == SDLK_3) {
					animator_change_animation(&anim, fox.animations_count < 4 ? NULL : &fox.animations[3]);
				} else if (event.key.key == SDLK_4) {
					animator_change_animation(&anim, fox.animations_count < 5 ? NULL : &fox.animations[4]);
				} else if (event.key.key == SDLK_5) {
					animator_change_animation(&anim, fox.animations_count < 6 ? NULL : &fox.animations[5]);
				} else if (event.key.key == SDLK_6) {
					animator_change_animation(&anim, fox.animations_count < 7 ? NULL : &fox.animations[6]);
				} else if (event.key.key == SDLK_7) {
					animator_change_animation(&anim, fox.animations_count < 8 ? NULL : &fox.animations[7]);
				} else if (event.key.key == SDLK_8) {
					animator_change_animation(&anim, fox.animations_count < 9 ? NULL : &fox.animations[8]);
				} else if (event.key.key == SDLK_9) {
					animator_change_animation(&anim, fox.animations_count < 10 ? NULL : &fox.animations[9]);
				} else if (event.key.key == SDLK_C) {
					is_shader_color = !is_shader_color;
				} else if (event.key.key == SDLK_V) {
					is_shader_lit = !is_shader_lit;
				} else if (event.key.key == SDLK_G) {
					is_grabbed = !is_grabbed;
					window_input_grab(window, is_grabbed);
				}
				break;
			default:
				break;
			}
		}
		const bool *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_W])
			cameraPos = glms_vec3_add(cameraPos, glms_vec3_scale(cameraFront, cameraSpeed));
		if (keys[SDL_SCANCODE_S])
			cameraPos = glms_vec3_sub(cameraPos, glms_vec3_scale(cameraFront, cameraSpeed));
		if (keys[SDL_SCANCODE_A])
			cameraPos = glms_vec3_sub(cameraPos, glms_vec3_scale(glms_vec3_normalize(glms_vec3_cross(cameraFront, cameraUp)), cameraSpeed));
		if (keys[SDL_SCANCODE_D])
			cameraPos = glms_vec3_add(cameraPos, glms_vec3_scale(glms_vec3_normalize(glms_vec3_cross(cameraFront, cameraUp)), cameraSpeed));
		if (keys[SDL_SCANCODE_SPACE])
			cameraPos = glms_vec3_add(cameraPos, glms_vec3_scale(glms_vec3_normalize(glms_vec3_cross(glms_vec3_cross(cameraFront, cameraUp), cameraFront)), cameraSpeed));
		if (keys[SDL_SCANCODE_LSHIFT])
			cameraPos = glms_vec3_sub(cameraPos, glms_vec3_scale(glms_vec3_normalize(glms_vec3_cross(glms_vec3_cross(cameraFront, cameraUp), cameraFront)), cameraSpeed));

		glClearColor(0.7f, 0.9f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		double new_time = get_time_sec();
		animator_update(&anim, new_time-last_time);

		struct shader *cur_shader = &fshader[is_shader_lit][is_shader_color];
		shader_use(cur_shader);
		mat4 model = GLM_MAT4_IDENTITY_INIT; // make sure to initialize matrix to identity matrix first
		mat4 view = GLM_MAT4_IDENTITY_INIT;
		mat4 projection = GLM_MAT4_IDENTITY_INIT;

		glm_translate(model, (vec3){0.0, 0.0, -3.0});
		glm_rotate_x(model, angleX, model);
		glm_rotate_y(model, angleY, model);

		// Fox
		//glm_scale(model, (vec3){0.02, 0.02, 0.02});
		// CesiumMan
		//glm_scale(model, (vec3){1, 1, 1});
		glm_scale(model, (vec3){model_scale, model_scale, model_scale});
#if 0
		float ratio = (float)scr_width / (float)scr_height;
		glm_ortho(-ratio, ratio, -1.f, 1.f, 0.1f, 100.0f, projection);
#else
		glm_perspective(glm_rad(45.0f), (float)scr_width / (float)scr_height, 0.1f, 100.0f, projection);
#endif
		glm_lookat(cameraPos.raw, glms_vec3_add(cameraPos, cameraFront).raw, cameraUp.raw, view);

		unsigned int projLoc = glGetUniformLocation(cur_shader->id, "projection");
		unsigned int modelLoc = glGetUniformLocation(cur_shader->id, "model");
		unsigned int viewLoc = glGetUniformLocation(cur_shader->id, "view");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);

		glUniform3f(glGetUniformLocation(cur_shader->id, "lightColor"), 1.0f, 1.0f, 1.0f);
		glUniform3f(glGetUniformLocation(cur_shader->id, "lightPos"), lightPos.raw[0], lightPos.raw[1], lightPos.raw[2]);
		glUniform3f(glGetUniformLocation(cur_shader->id, "viewPos"), cameraPos.raw[0], cameraPos.raw[1], cameraPos.raw[2]);

		glEnable(GL_DEPTH_TEST);
		model_draw(&fox, &anim, cur_shader);
		SDL_GL_SwapWindow(window);
		last_time = new_time;
	}

	animator_cleanup(&anim);
	model_cleanup(&fox);
	shader_cleanup(&fshader[0][0]);
	shader_cleanup(&fshader[0][1]);
	shader_cleanup(&fshader[1][0]);
	shader_cleanup(&fshader[1][1]);

	return 0;
}
