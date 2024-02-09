#include "window.h"
#include "../util/debug.h"

#include <SDL_vulkan.h>

namespace engine {
	Window::Window() {

	}

	void Window::init(const char* appName) {
		// Initialize SDL
		SDL_Init(SDL_INIT_VIDEO);

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

		mWindow = SDL_CreateWindow(
			appName,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			mWindowExtent.width,
			mWindowExtent.height,
			window_flags
		);
	}

	void Window::handleEvents() {
		SDL_Event e;

		// Handle SDL events
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			else if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					minimized = true;
				}
				else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					minimized = false;
				}
			}
		}
	}

	void Window::cleanup() {
		SDL_DestroyWindow(mWindow);
	}

	std::vector<const char*> Window::getRequiredSDLExtensions() const {
		unsigned int count;

		SDL_Vulkan_GetInstanceExtensions(mWindow, &count, NULL);

		std::vector<const char*> requiredExtensions(count);

		SDL_Vulkan_GetInstanceExtensions(mWindow, &count, requiredExtensions.data());

		return requiredExtensions;
	}
}