#include "window.h"
#include "../util/debug.h"

#include <SDL_vulkan.h>

namespace engine {
	Window* pMainWindow = nullptr;

	Window::Window() {
		if (pMainWindow == nullptr) {
			pMainWindow = this;
		}
		else {
			util::displayError("Cannot have more than one window");
		}
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
			else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.scancode) {
				case SDL_SCANCODE_W:
					holdingW = true;
					break;
				case SDL_SCANCODE_A:
					holdingA = true;
					break;
				case SDL_SCANCODE_S:
					holdingS = true;
					break;
				case SDL_SCANCODE_D:
					holdingD = true;
					break;
				case SDL_SCANCODE_LCTRL:
					holdingCTRL = true;
					break;
				case SDL_SCANCODE_SPACE:
					holdingSpace = true;
					break;
				case SDL_SCANCODE_LEFT:
					holdingLeft = true;
					break;
				case SDL_SCANCODE_RIGHT:
					holdingRight = true;
					break;
				}
			}
			else if (e.type == SDL_KEYUP) {
				switch (e.key.keysym.scancode) {
				case SDL_SCANCODE_W:
					holdingW = false;
					break;
				case SDL_SCANCODE_A:
					holdingA = false;
					break;
				case SDL_SCANCODE_S:
					holdingS = false;
					break;
				case SDL_SCANCODE_D:
					holdingD = false;
					break;
				case SDL_SCANCODE_LCTRL:
					holdingCTRL = false;
					break;
				case SDL_SCANCODE_SPACE:
					holdingSpace = false;
					break;
				case SDL_SCANCODE_LEFT:
					holdingLeft = false;
					break;
				case SDL_SCANCODE_RIGHT:
					holdingRight = false;
					break;
				}
			}
		}
	}

	void Window::cleanup() {
		SDL_DestroyWindow(mWindow);
	}

	Window& Window::getMainWindow() {
		return *pMainWindow;
	}

	std::vector<const char*> Window::getRequiredSDLExtensions() const {
		unsigned int count;

		SDL_Vulkan_GetInstanceExtensions(mWindow, &count, NULL);

		std::vector<const char*> requiredExtensions(count);

		SDL_Vulkan_GetInstanceExtensions(mWindow, &count, requiredExtensions.data());

		return requiredExtensions;
	}
}