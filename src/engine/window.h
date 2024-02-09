#pragma once

#include <SDL.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace engine {
	class Window {
	public:
		Window();

		void init(const char* appName);

		void handleEvents();

		void cleanup();

		SDL_Window* getWindow() { return mWindow; }

		VkExtent2D getExtent() { return mWindowExtent; }

		bool isMinimized() { return minimized; }
		bool shouldQuit() { return quit; }

		std::vector<const char*> getRequiredSDLExtensions() const;
	private:
		VkExtent2D mWindowExtent{ 1700, 900 };
		SDL_Window* mWindow{ nullptr };

		bool minimized{ false };
		bool quit{ false };
	};
}