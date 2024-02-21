#pragma once

#include "memory_management.h"
#include "renderer.h"

namespace engine {
	namespace rendering {
		bool loadImageFromFile(Renderer& renderer, const char* file, AllocatedImage& outImage);
	}
}