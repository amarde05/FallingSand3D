#pragma once

#include "window.h"
#include "rendering/renderer.h"

#include <vulkan/vulkan.h>

#include <SDL.h>

#include <vector>
#include <memory>

#define ENGINE_NAME "Voxel Engine"

namespace engine {
	class VulkanEngine {
	public:
		static VulkanEngine& Get();

		VulkanEngine(const char* name);

		void init();

		void cleanup();

		void run();
	private:
		bool mIsInitialized{ false };
		bool mStopRendering{ false };

		const char* mApplicationName;

		Window mWindow;
		rendering::Renderer mRenderer;
	};
}