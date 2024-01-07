#include "Texture.h"

// stbi library spams compiler warnings due to some black magic it does
#pragma warning(push)
#pragma warning(disable : 26451; disable : 26819; disable : 6262; disable : 26450; disable : 6001)
#pragma warning(disable : 6293; disable : 26454; disable : 6385; disable : 33011)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
#pragma warning(pop)

#include "Log.h"

bool Texture::Exists() const {
	return data.size() != 0;
}

Color Texture::SampleUVClamp(const glm::vec2& uv) const {
	const glm::ivec2 pt = glm::clamp((glm::ivec2)(glm::fract(uv) * sizeF), glm::ivec2(0, 0), size);
	return data[pt.x + size.x * pt.y];
}

Texture::Texture(const std::filesystem::path& path, bool flip) {

	stbi_set_flip_vertically_on_load(flip);

	int numChannels;
	stbi_uc* img = stbi_load(path.string().c_str(), &size.x, &size.y, &numChannels, 4); // Request RGBA texture
	sizeF = size;
	if (img == nullptr) LOG_FATAL_AND_EXIT_ARG("Failed to load img: ", stbi_failure_reason());
	if (size.x == 0 || size.y == 0) LOG_FATAL_AND_EXIT("Zero size image?");

	static_assert(sizeof(Color) == 4); // Make sure no overflow if color struct changes

	constexpr int MAX_IMG_SIZE = 512;

	// Resize image
	if (size.x > MAX_IMG_SIZE) {
		double ratio = MAX_IMG_SIZE / (double)size.x;
		int newWidth = (int)(ratio * size.x), newHeight = (int)(ratio * size.y);

		data.resize(newWidth * newHeight);
		stbir_resize(img, size.x, size.y, 0, (unsigned char*)data.data(), newWidth, newHeight, 0, stbir_pixel_layout::STBIR_RGBA,
			stbir_datatype::STBIR_TYPE_UINT8, stbir_edge::STBIR_EDGE_CLAMP, stbir_filter::STBIR_FILTER_DEFAULT);
		size.x = newWidth;
		size.y = newHeight;
		sizeF = size;
	}
	else {
		// Copies data, oof
		data.resize(size.x * size.y);
		memcpy(data.data(), img, size.x * size.y * 4);
	}


	stbi_image_free(img);

	fmt::println("Read {}, Size {}x{}", path.string(), size.x, size.y);
}

