#pragma once

#include "../window.h"
#include "device.h"
#include "descriptors.h"
#include "commands.h"
#include "deletion_queue.h"
#include "memory_management.h"
#include "mesh.h"

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <SDL.h>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace engine {
	namespace rendering {
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

		struct MeshPushConstants {
			glm::vec4 data;
			glm::mat4 renderMatrix;
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

			Mesh mMesh;

			VkImageView mDepthImageView;
			AllocatedImage mDepthImage;
			VkFormat mDepthFormat;

			std::unique_ptr<VulkanDescriptorPool> mGlobalPool;

			VkDescriptorSetLayout mDescriptorSetLayout;

			FrameData mFrames[FRAME_OVERLAP];

			VmaAllocator mAllocator;
			
			DeletionQueue mMainDeletionQueue;
			DeletionQueue mAllocationDeletionQueue;


			void createInstance(VkApplicationInfo appInfo);
			void setupDebugMessenger();
			void createSurface();

			void createAllocator();

			void createSwapchain();
			void createSwapchainImageViews();
			void cleanupSwapchain();

			void allocateCommandBuffers();

			void createRenderPass();
			void createFramebuffers();

			void createSyncStructures();

			void createPipelines();

			void loadMeshes();

			void createDescriptorSetLayout();

			bool checkValidationLayerSupport() const;
			bool checkInstanceExtensionSupport(std::vector<const char*>& extensions) const;

			bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const;
			void uploadMesh(Mesh& mesh);


			FrameData& getCurrentFrame() { return mFrames[mFrameNumber % FRAME_OVERLAP]; }
		};
	}
}