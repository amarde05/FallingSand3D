#include "engine.h"
#include "../util/debug.h"

#include <chrono>
#include <thread>



namespace engine {
	VulkanEngine* loadedEngine = nullptr;

	VulkanEngine& VulkanEngine::Get() {
		return *loadedEngine;
	}

	VulkanEngine::VulkanEngine(const char* name) : mApplicationName{ name }, mRenderer{ rendering::Renderer(&mWindow) }, mWindow{ Window() } {
	}

	void VulkanEngine::init() {
		// Ensure there is only one instance of VulkanEngine
		if (loadedEngine != nullptr)
			util::displayError("Can't have more than one engine.");

		loadedEngine = this;

		
		mWindow.init(mApplicationName);

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = mApplicationName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = ENGINE_NAME;
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		mRenderer.init(appInfo);

		// Everything is initialized, so set mIsInitialized to true
		mIsInitialized = true;
	}

	void VulkanEngine::cleanup() {
		if (mIsInitialized) {
			mRenderer.cleanup();

			mWindow.cleanup();
		}

		// Engine is no longer loaded, so remove pointer
		loadedEngine = nullptr;
	}

	void VulkanEngine::run() {
		if (!mIsInitialized)
			return;

		// Main loop
		while (!mWindow.shouldQuit()) {
			mWindow.handleEvents();

			// Don't draw if minimized
			if (mWindow.isMinimized()) {
				// Throttle speed to avoid endless spinning
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			else {
				mRenderer.draw();
			}
		}

		// Ensure that no more graphics commands are being run
		mRenderer.waitForGraphics();
	}
}