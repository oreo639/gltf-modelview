#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "image.h"

bool image_load_file(struct image_tex *image, const char *fpath) {
	int width, height, nrChannels;
	image->data = stbi_load(fpath, &width, &height, &nrChannels, 0);	
	image->width = width;
	image->height = height;
	if (nrChannels == 1)
		image->fmt = image_texture_bw;
	else if (nrChannels == 3)
		image->fmt = image_texture_rgb;
	else if (nrChannels == 4)
		image->fmt = image_texture_rgba;

	return true;
}

bool image_load_mem(struct image_tex *image, const unsigned char *data, size_t size) {
	int width, height, nrChannels;
	image->data = stbi_load_from_memory(data, size, &width, &height, &nrChannels, 0);	
	image->width = width;
	image->height = height;
	if (nrChannels == 1)
		image->fmt = image_texture_bw;
	else if (nrChannels == 3)
		image->fmt = image_texture_rgb;
	else if (nrChannels == 4)
		image->fmt = image_texture_rgba;

	return true;
}

void image_free(struct image_tex *image) {
	stbi_image_free(image->data);
}
