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
	int x = glm::clamp((int)(uv.x * width), 0, width);
	int y = glm::clamp((int)(uv.y * height), 0, height);
	return data[x + width * y];
}

Texture::Texture(const std::filesystem::path& path, bool flip) {

	stbi_set_flip_vertically_on_load(flip);

	int numChannels;
	stbi_uc* img = stbi_load(path.string().c_str(), &width, &height, &numChannels, 4); // Request RGBA texture
	if (img == nullptr) {
		Log::FatalError("Failed to load img: ", stbi_failure_reason());
		return;
	}
	if (width == 0 || height == 0) Log::FatalError("Zero size image?");

	static_assert(sizeof(Color) == 4); // Make sure no overflow if color struct changes

	// Resize image
	if (width > 4096) {
		double ratio = 4096 / (double)width;
		int newWidth = (int)(ratio * width), newHeight = (int)(ratio * height);

		data.resize(newWidth * newHeight);
		stbir_resize(img, width, height, 0, (unsigned char*)data.data(), newWidth, newHeight, 0, stbir_pixel_layout::STBIR_RGBA,
			stbir_datatype::STBIR_TYPE_UINT8, stbir_edge::STBIR_EDGE_CLAMP, stbir_filter::STBIR_FILTER_DEFAULT);
		width = newWidth;
		height = newHeight;
	}
	else {
		// Copies data, oof
		data.resize(width * height);
		memcpy(data.data(), img, width * height * 4);
	}

	stbi_image_free(img);

	Log::LineFormatted("Read {}, Size {}x{}", path.string(), width, height);
}

