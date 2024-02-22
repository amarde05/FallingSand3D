#pragma once

#include "memory/memory_management.h"
#include "renderer.h"

#include <string>

namespace engine {
	namespace rendering {
		struct Texture {
			memory::AllocatedImage image;
			VkImageView imageView;
		};

		void addTexture(std::string name, Texture& tex);

		Texture* getTexture(const std::string& name);

		bool loadImageFromFile(Renderer& renderer, const char* file, memory::AllocatedImage& outImage);
	}
}