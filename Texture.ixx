module;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>

export module Texture;

import <memory>;
import <filesystem>;
import <fstream>;
import <vector>;
import Utils;

export class Texture {
public:
	Texture() : data() {  }

	// Data gets converted to standard Color vector on load
	std::vector<Color> data;
	int width, height;

	bool Exists() const {
		return data.size() != 0;
	}

	Color SampleUVClamp(const glm::vec2& uv) const {
		int x = glm::clamp((int)((uv.x) * width ), 0, width);
		int y = glm::clamp((int)((1.0f - uv.y) * height), 0, height);
		return data[x + width * y];
	}

	Texture(const std::filesystem::path& path) {

		int numChannels;
		stbi_uc* img = stbi_load(path.string().c_str(), &width, &height, &numChannels, 4); // Request RGBA texture
		if (img == nullptr) Log::FatalError("Failed to load img: ", stbi_failure_reason());
		if (width == 0 || height == 0) Log::FatalError("Zero size image?");

		static_assert(sizeof(Color) == 4); // Make sure no overflow if color changes

		// Copies data, oof
		data.resize(width * height);
		memcpy(data.data(), img, width * height * 4);

		stbi_image_free(img);

		Log::LineFormatted("Read {}, Size {}x{}", path.string(), width, height);
	}
};