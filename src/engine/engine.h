#pragma once
#include "device.h"
#include "descriptors.h"
#include "commands.h"
#include "deletion_queue.h"

#include <vulkan/vulkan.h>

#include <SDL.h>

#include <vector>
#include <memory>

#define ENGINE_NAME "Voxel Engine"

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

	class VulkanEngine {
	public:
		static VulkanEngine& Get();

		VulkanEngine(const char* name) : mApplicationName{ name } {}

		void init();

		void cleanup();

		void draw();

		void run();
	private:
		bool mIsInitialized{ false };
		bool mStopRendering{ false };
		int mFrameNumber{ 0 };

		VkExtent2D mWindowExtent{ 1700, 900 };
		SDL_Window* mWindow{ nullptr };

		const char* mApplicationName;

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


		void createInstance();
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
		std::vector<const char*> getRequiredSDLExtensions() const;

		bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const;

		FrameData& getCurrentFrame() { return mFrames[mFrameNumber % FRAME_OVERLAP]; }
	};
}