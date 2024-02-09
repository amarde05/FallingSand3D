#pragma once

#include <SDL.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace engine {
	class Window {
	public:
		std::vector<const char*> getRequiredSDLExtensions() const;
	private:
		VkExtent2D mWindowExtent{ 1700, 900 };
		SDL_Window* mWindow{ nullptr };
	};
}