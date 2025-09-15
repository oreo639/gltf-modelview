#pragma once

#include <stdbool.h>

enum image_texture_format {
	image_texture_bw,
	image_texture_rgb,
	image_texture_rgba,
};

struct image_tex {
	unsigned char *data;
	unsigned width;
	unsigned height;
	enum image_texture_format fmt;
};

bool image_load_file(struct image_tex *image, const char *fpath);
bool image_load_mem(struct image_tex *image, const unsigned char *data, size_t size);
void image_free(struct image_tex *image);
