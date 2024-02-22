#pragma once

#include "../window.h"
#include "device.h"
#include "descriptors.h"
#include "commands.h"
#include "deletion_queue.h"
#include "memory/memory_management.h"
#include "mesh.h"

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <SDL.h>

#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

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

			memory::AllocatedBuffer objectBuffer;
			VkDescriptorSet objectDescriptor;
		};

		struct GPUCameraData {
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
		};

		struct GPUSceneData {
			glm::vec4 fogColor; // w is for exponent
			glm::vec4 fogDistances; // x for min, y for max, zw unused
			glm::vec4 ambientColor;
			glm::vec4 sunlightDirection; // w for sun power
			glm::vec4 sunlightColor;
		};

		struct GPUObjectData {
			glm::mat4 modelMatrix;
		};

		struct MeshPushConstants {
			glm::vec4 data;
			glm::mat4 renderMatrix;
		};

		struct RenderObject {
			Mesh* mesh;

			class Material* material;

			glm::mat4 transformMatrix;
		};

		struct UploadContext {
			VkFence uploadFence;
			std::unique_ptr<VulkanCommandPool> commandPool;
			VkCommandBuffer commandBuffer;
		};

		class Renderer {
		public:
			Renderer(Window* window);

			void init(VkApplicationInfo appInfo);

			void cleanup();

			void draw();

			void waitForGraphics();

		
			void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

			VulkanDevice& getDevice() { return *mDevice.get(); };

			DeletionQueue& getMainDeletionQueue() { return mMainDeletionQueue; }
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
			Mesh mTriangleMesh;
			Mesh mTeapotMesh;
			Mesh mVikingRoom;

			VkImageView mDepthImageView;
			memory::AllocatedImage mDepthImage;
			VkFormat mDepthFormat;

			VkDescriptorSetLayout mGlobalSetLayout;
			VkDescriptorSetLayout mObjectSetLayout;
			VkDescriptorSetLayout mSingleTextureLayout;
			VkDescriptorPool mDescriptorPool;

			VkDescriptorSet mGlobalDescriptor;

			FrameData mFrames[FRAME_OVERLAP];

			DeletionQueue mMainDeletionQueue;

			std::vector<RenderObject> mRenderables;

			std::unordered_map<std::string, Mesh> mMeshes;

			glm::vec3 camPos {0, -6, -10};

			GPUSceneData mSceneParameters;
			memory::AllocatedBuffer mGlobalBuffer;

			UploadContext mUploadContext;


			void createInstance(VkApplicationInfo appInfo);
			void setupDebugMessenger();
			void createSurface();

			void createSwapchain();
			void createSwapchainImageViews();
			void cleanupSwapchain();

			void initCommands();

			void createRenderPass();
			void createFramebuffers();

			void createSyncStructures();

			void initDescriptors();
			void createPipelines();
			void initMaterials();
			
			void loadMeshes();

			void loadTextures();

			void initScene();

			


			void drawObjects(VkCommandBuffer cmd, RenderObject* first, int count);


			bool checkValidationLayerSupport() const;
			bool checkInstanceExtensionSupport(std::vector<const char*>& extensions) const;

			size_t padUniformBufferSize(size_t originalSize) const;

			bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const;
			
			void uploadMesh(Mesh& mesh);
			Mesh* getMesh(const std::string& name);

			FrameData& getCurrentFrame() { return mFrames[mFrameNumber % FRAME_OVERLAP]; }
		};
	}
}