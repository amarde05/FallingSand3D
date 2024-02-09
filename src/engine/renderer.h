#pragma once

#include "window.h"
#include "device.h"
#include "descriptors.h"
#include "commands.h"
#include "deletion_queue.h"

#include <vulkan/vulkan.h>

#include <SDL.h>

#include <vector>
#include <memory>

namespace engine {
#ifdef NODEBUG
	const bool ENABLE_VALIDATION_LAYERS = false;
#else
	const bool ENABLE_VALIDATION_LAYERS = true;
#endif

#define FRAME_OVERLAP 2

	struct FrameData {
		VkCommandBuffer frameCommandBuffer;

		VkFence renderFence;
		VkSemaphore presentSemaphore, renderSemaphore;
	};

	class Renderer {
	public:
		Renderer(Window* window);

		void init(VkApplicationInfo appInfo);

		void cleanup();

		void draw();

		void waitForGraphics();
	private:
		bool mStopRendering{ false };
		int mFrameNumber{ 0 };

		Window* mWindow;

		DeletionQueue mDeletionQueue;

		// Vulkan objects
		VkInstance mInstance;
		VkDebugUtilsMessengerEXT mDebugMessenger;
		std::unique_ptr<VulkanDevice> mDevice;
		VkSurfaceKHR mSurface;

		VkSwapchainKHR mSwapchain;
		VkExtent2D mSwapchainExtent;
		VkFormat mSwapchainImageFormat;

		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainImageViews;
		std::vector<VkFramebuffer> mSwapchainFramebuffers;

		VkRenderPass mRenderPass;

		VkPipelineLayout mTrianglePipelineLayout;
		VkPipeline mTrianglePipeline;

		std::unique_ptr<VulkanDescriptorPool> mGlobalPool;

		VkDescriptorSetLayout mDescriptorSetLayout;

		// Frame objects
		FrameData mFrames[FRAME_OVERLAP];


		void createInstance(VkApplicationInfo appInfo);
		void setupDebugMessenger();
		void createSurface();

		void createSwapchain();
		void createSwapchainImageViews();
		void cleanupSwapchain();

		void allocateCommandBuffers();

		void createRenderPass();
		void createFramebuffers();

		void createSyncStructures();

		void createPipelines();

		void createDescriptorSetLayout();

		bool checkValidationLayerSupport() const;
		bool checkInstanceExtensionSupport(std::vector<const char*>& extensions) const;

		bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const;

		FrameData& getCurrentFrame() { return mFrames[mFrameNumber % FRAME_OVERLAP]; }
	};
}