#include "renderer.h"
#include "pipelines.h"
#include "../../util/debug.h"

#include <SDL_vulkan.h>

#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <array>
#include <fstream>

namespace {
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

		std::string msg = "Validation layer: ";
		msg += pCallbackData->pMessage;

		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			util::displayMessage(msg, DISPLAY_TYPE_WARN);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			util::displayMessage(msg, DISPLAY_TYPE_ERR);
			break;
		default:
			util::displayMessage(msg, DISPLAY_TYPE_NONE);
			break;
		}

		return VK_FALSE;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	// Swapchain
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			SDL_GetWindowSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	// Images
	VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		util::displayError("Failed to find a supported format out of the list of candidates");
	}

	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
		return findSupportedFormat(physicalDevice, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}


namespace engine {
	namespace rendering {
		const std::vector<const char*> VALIDATION_LAYERS = {
			"VK_LAYER_KHRONOS_validation"
		};

		Renderer::Renderer(Window* window) {
			mWindow = window;
		}

		void Renderer::init(VkApplicationInfo appInfo) {
			// Call initialization functions
			createInstance(appInfo);
			setupDebugMessenger();
			createSurface();

			mDevice = std::make_unique<VulkanDevice>(mInstance, mSurface, ENABLE_VALIDATION_LAYERS, VALIDATION_LAYERS);

			createAllocator();

			createSwapchain();
			createSwapchainImageViews();

			allocateCommandBuffers();

			createRenderPass();
			createFramebuffers();

			createSyncStructures();

			initDescriptors();
			createPipelines();

			loadMeshes();

			initScene();

			mMainDeletionQueue.pushFunction([=]() { cleanupSwapchain(); });
		}

		void Renderer::draw() {
			float camMoveSpeed = 3.0f;
			if (mWindow->holdingW) {
				camPos.z += camMoveSpeed * 0.016f;
			}
			else if (mWindow->holdingS) {
				camPos.z -= camMoveSpeed * 0.016f;
			}

			if (mWindow->holdingA) {
				camPos.x += camMoveSpeed * 0.016f;
			}
			else if (mWindow->holdingD) {
				camPos.x -= camMoveSpeed * 0.016f;
			}

			if (mWindow->holdingSpace) {
				camPos.y -= camMoveSpeed * 0.016f;
			}
			else if (mWindow->holdingCTRL) {
				camPos.y += camMoveSpeed * 0.016f;
			}

			// Wait until the GPU has finished rendering the last frame. Timeout of 1 second
			vkWaitForFences(mDevice->getDevice(), 1, &getCurrentFrame().renderFence, true, 1000000000);
			vkResetFences(mDevice->getDevice(), 1, &getCurrentFrame().renderFence);

			// Request image from the swapchain. Timeout of 1 second
			uint32_t swapchainImageIndex;
			if (vkAcquireNextImageKHR(mDevice->getDevice(), mSwapchain, 1000000000, getCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex) != VK_SUCCESS) {
				util::displayError("Failed to acquire next swapchain image");
			}

			// Reset command buffer now that we know the GPU has finished rendering the last frame
			if (vkResetCommandBuffer(getCurrentFrame().frameCommandBuffer, 0) != VK_SUCCESS) {
				util::displayError("Failed to reset command buffer");
			}

			VkCommandBuffer cmd = getCurrentFrame().frameCommandBuffer;

			VkCommandBufferBeginInfo cmdBeginInfo{};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;

			cmdBeginInfo.pInheritanceInfo = nullptr;
			cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			if (vkBeginCommandBuffer(cmd, &cmdBeginInfo) != VK_SUCCESS) {
				util::displayError("Failed to begin command buffer");
			}

			VkClearValue clearValue;
			float flash = abs(sin(mFrameNumber / 360.0f));
			clearValue.color = { {0.0f, 0.0f, flash, 1.0f } };

			// Clear depth
			VkClearValue depthClear;
			depthClear.depthStencil.depth = 1.0f;

			// Start the main render pass
			VkRenderPassBeginInfo renderpassInfo{};
			renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpassInfo.pNext = nullptr;

			renderpassInfo.renderPass = mRenderPass;
			renderpassInfo.renderArea.offset.x = 0;
			renderpassInfo.renderArea.offset.y = 0;
			renderpassInfo.renderArea.extent = mWindow->getExtent();
			renderpassInfo.framebuffer = mSwapchainFramebuffers[swapchainImageIndex];

			// Connect clear values
			VkClearValue clearValues[2]{ clearValue, depthClear };
			renderpassInfo.clearValueCount = 2;
			renderpassInfo.pClearValues = clearValues;

			// Begin the render pass
			vkCmdBeginRenderPass(cmd, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

			drawObjects(cmd, mRenderables.data(), mRenderables.size());

			// Finalize the render pass
			vkCmdEndRenderPass(cmd);

			// Finalize the command buffer
			if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
				util::displayError("Failed to end renderpass");
			}

			// Prepare for submission to the queue
			// Wait on the present semaphore to know when the swapchain is ready
			// Signal the render semaphore to signal that rendering has finished

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			submitInfo.pWaitDstStageMask = &waitStage;

			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &getCurrentFrame().presentSemaphore;

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &getCurrentFrame().renderSemaphore;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmd;

			// Submit command buffer to the queue and execute it
			// The render fence will block until graphics commands finish
			if (vkQueueSubmit(mDevice->getGraphicsQueue(), 1, &submitInfo, getCurrentFrame().renderFence) != VK_SUCCESS) {
				util::displayError("Failed to submit to queue");
			}

			// Display the newly-rendered image to the window
			// Wait on the render semaphore
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;

			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &mSwapchain;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;

			presentInfo.pImageIndices = &swapchainImageIndex;

			if (vkQueuePresentKHR(mDevice->getGraphicsQueue(), &presentInfo) != VK_SUCCESS) {
				util::displayError("Failed to queue present");
			}

			// Increase the number of frames drawn
			mFrameNumber++;
		}

		void Renderer::cleanup() {
			mAllocationDeletionQueue.flush();
			
			vmaDestroyAllocator(mAllocator);
			
			mMainDeletionQueue.flush();

			mDevice = nullptr;

			vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

			if (ENABLE_VALIDATION_LAYERS) {
				DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
			}

			vkDestroyInstance(mInstance, nullptr);
		}

		void Renderer::waitForGraphics() {
			vkQueueWaitIdle(mDevice->getGraphicsQueue());
		}

		void Renderer::drawObjects(VkCommandBuffer cmd, RenderObject* first, int count) {
			glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);
			glm::mat4 proj = glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.01f, 200.0f);
			proj[1][1] *= -1;

			glm::mat4 viewproj = proj * view;

			GPUCameraData camData;
			camData.proj = proj;
			camData.view = view;
			camData.viewproj = viewproj;

			// Copy data to the buffer
			void* data;
			vmaMapMemory(mAllocator, getCurrentFrame().cameraBuffer.allocation, &data);

			memcpy(data, &camData, sizeof(GPUCameraData));

			vmaUnmapMemory(mAllocator, getCurrentFrame().cameraBuffer.allocation);

			float framed = (mFrameNumber / 120.0f);

			mSceneParameters.ambientColor = { sin(framed), 0, cos(framed), 1 };

			char* sceneData;
			vmaMapMemory(mAllocator, mSceneParameterBuffer.allocation, (void**)&sceneData);

			int frameIndex = mFrameNumber % FRAME_OVERLAP;

			sceneData += padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

			memcpy(sceneData, &mSceneParameters, sizeof(GPUSceneData));

			vmaUnmapMemory(mAllocator, mSceneParameterBuffer.allocation);


			Mesh* lastMesh = nullptr;
			Material* lastMaterial = nullptr;

			glm::mat4 rotation = glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(1, 0, 0))*
				glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(0, 1, 0))*
				glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(0, 0, 1));

			for (int i = 0; i < count; i++) {
				RenderObject& object = first[i];

				// Only bind the pipeline if it doesn't match with the already bount one
				if (object.material != lastMaterial) {
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
					lastMaterial = object.material;

					uint32_t uniformOffset = padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

					// Bind the descriptor set when changing pipeline
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &getCurrentFrame().globalDescriptor, 1, &uniformOffset);
				}

				MeshPushConstants constants;

				constants.renderMatrix = object.transformMatrix * rotation;

				vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
				
				// Only bind the mesh if it's a different one from last bind
				if (object.mesh != lastMesh) {
					VkDeviceSize offset = 0;
					vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
					lastMesh = object.mesh;
				}

				vkCmdDraw(cmd, object.mesh->vertices.size(), 1, 0, 0);
			}
		}

		// Vulkan initialization functions
		void Renderer::createInstance(VkApplicationInfo appInfo) {
			if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
				util::displayError("Could not create vulkan instance because required validation layers are not supported");
			}

			VkInstanceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;

			std::vector<const char*> requiredExtensions = mWindow->getRequiredSDLExtensions();

			if (ENABLE_VALIDATION_LAYERS) {
				requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			if (!checkInstanceExtensionSupport(requiredExtensions)) {
				util::displayError("Could not create vulkan instance because required instance extensions not supported");
			}

			requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

			createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
			createInfo.ppEnabledExtensionNames = requiredExtensions.data();

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			if (ENABLE_VALIDATION_LAYERS) {
				createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
				createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

				populateDebugMessengerCreateInfo(debugCreateInfo);
				createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
			}
			else {
				createInfo.enabledLayerCount = 0;

				createInfo.pNext = nullptr;
			}

			if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
				util::displayError("Could not create vulkan instance");
			}
		}

		void Renderer::setupDebugMessenger() {
			if (!ENABLE_VALIDATION_LAYERS) return;

			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			populateDebugMessengerCreateInfo(createInfo);

			if (CreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
				util::displayError("Failed to create debug messenger");
			}
		}

		void Renderer::createSurface() {
			if (SDL_Vulkan_CreateSurface(mWindow->getWindow(), mInstance, &mSurface) != SDL_TRUE) {
				util::displayError("Failed to create surface!");
			}
		}

		void Renderer::createAllocator() {
			VmaAllocatorCreateInfo allocatorInfo{};
			allocatorInfo.physicalDevice = mDevice->getPhysicalDevice();
			allocatorInfo.device = mDevice->getDevice();
			allocatorInfo.instance = mInstance;
			vmaCreateAllocator(&allocatorInfo, &mAllocator);
		}

		void Renderer::createSwapchain() {
			SwapchainSupportDetails swapChainSupport = mDevice->getSwapchainSupport();

			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
			VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, mWindow->getWindow());

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

			if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
				imageCount = swapChainSupport.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = mSurface;

			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			QueueFamilyIndices indices = mDevice->getQueueFamilyIndices();
			uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

			if (indices.graphicsFamily != indices.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = nullptr;
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;

			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(mDevice->getDevice(), &createInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
				util::displayError("Failed to create swapchain");
			}

			vkGetSwapchainImagesKHR(mDevice->getDevice(), mSwapchain, &imageCount, nullptr);
			mSwapchainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(mDevice->getDevice(), mSwapchain, &imageCount, mSwapchainImages.data());

			mSwapchainImageFormat = surfaceFormat.format;
			mSwapchainExtent = extent;


			// Create depth buffer
			VkExtent3D depthImageExtent = {
				mWindow->getExtent().width,
				mWindow->getExtent().height,
				1
			};

			mDepthFormat = findDepthFormat(mDevice->getPhysicalDevice());

			VkImageCreateInfo dimgInfo{};
			dimgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			dimgInfo.pNext = nullptr;

			dimgInfo.imageType = VK_IMAGE_TYPE_2D;
			dimgInfo.format = mDepthFormat;
			dimgInfo.extent = depthImageExtent;

			dimgInfo.mipLevels = 1;
			dimgInfo.arrayLayers = 1;
			dimgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			dimgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			dimgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

			VmaAllocationCreateInfo dimgAllocInfo{};
			dimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			dimgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			vmaCreateImage(mAllocator, &dimgInfo, &dimgAllocInfo, &mDepthImage.image, &mDepthImage.allocation, nullptr);

			VkImageViewCreateInfo dviewInfo{};
			dviewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			dviewInfo.pNext = nullptr;

			dviewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			dviewInfo.image = mDepthImage.image;
			dviewInfo.format = mDepthFormat;
			dviewInfo.subresourceRange.baseMipLevel = 0;
			dviewInfo.subresourceRange.levelCount = 1;
			dviewInfo.subresourceRange.baseArrayLayer = 0;
			dviewInfo.subresourceRange.layerCount = 1;
			dviewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (vkCreateImageView(mDevice->getDevice(), &dviewInfo, nullptr, &mDepthImageView) != VK_SUCCESS) {
				util::displayError("Failed to create depth image view");
			}

			mAllocationDeletionQueue.pushFunction([=]() {
				vkDestroyImageView(mDevice->getDevice(), mDepthImageView, nullptr);
				vmaDestroyImage(mAllocator, mDepthImage.image, mDepthImage.allocation);
			});
		}

		void Renderer::createSwapchainImageViews() {
			mSwapchainImageViews.resize(mSwapchainImages.size());

			for (size_t i = 0; i < mSwapchainImages.size(); i++) {
				mDevice->createImageView(mSwapchainImages[i], mSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, mSwapchainImageViews[i]);
			}
		}


		void Renderer::allocateCommandBuffers() {
			for (int i = 0; i < FRAME_OVERLAP; i++) {
				mDevice->getGraphicsPool().allocateBuffers(&mFrames[i].frameCommandBuffer, 1);
			}
		}


		void Renderer::createRenderPass() {
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = mSwapchainImageFormat;
			//colorAttachment.samples = mDevice->getDeviceProperties().maxSamples;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription colorAttachmentResolve{};
			colorAttachmentResolve.format = mSwapchainImageFormat;
			colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentResolveRef{};
			colorAttachmentResolveRef.attachment = 2;
			colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = mDepthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//mDevice->getDeviceProperties().maxSamples;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef{};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			//subpass.pResolveAttachments = &colorAttachmentResolveRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;



			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			
			VkSubpassDependency depthDependency{};
			depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			depthDependency.dstSubpass = 0;
			depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			depthDependency.srcAccessMask = 0;
			depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment/*, colorAttachmentResolve*/};
			VkSubpassDependency dependencies[2]{ dependency, depthDependency };


			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();//attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies;

			if (vkCreateRenderPass(mDevice->getDevice(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
				util::displayError("Failed to create renderpass");
			}

			mMainDeletionQueue.pushFunction([=]() {
				vkDestroyRenderPass(mDevice->getDevice(), mRenderPass, nullptr);
			});
		}

		void Renderer::createFramebuffers() {
			// Create framebuffers for the swapchain images
			// This will connect the renderpass to the images for rendering
			VkFramebufferCreateInfo frameBufferInfo{};
			frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferInfo.pNext = nullptr;

			frameBufferInfo.renderPass = mRenderPass;
			frameBufferInfo.attachmentCount = 1;
			frameBufferInfo.width = mWindow->getExtent().width;
			frameBufferInfo.height = mWindow->getExtent().height;
			frameBufferInfo.layers = 1;

			const uint32_t swapchainImageCount = mSwapchainImages.size();
			mSwapchainFramebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

			// Create framebuffers for each of the swapchain image views
			for (int i = 0; i < swapchainImageCount; i++) {
				VkImageView attachments[2];
				attachments[0] = mSwapchainImageViews[i];
				attachments[1] = mDepthImageView;

				frameBufferInfo.attachmentCount = 2;
				frameBufferInfo.pAttachments = attachments;

				if (vkCreateFramebuffer(mDevice->getDevice(), &frameBufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS) {
					util::displayError("Failed to create framebuffers");
				}
			}
		}


		void Renderer::createSyncStructures() {
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.pNext = nullptr;

			// Create the fence with the VK_FENCE_CREATE_SIGNALED_BIT flag so it can be waited on before being used by a GPU Command
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = 0;

			// Create the sync structures for each frame
			for (int i = 0; i < FRAME_OVERLAP; i++) {
				if (vkCreateFence(mDevice->getDevice(), &fenceCreateInfo, nullptr, &mFrames[i].renderFence) != VK_SUCCESS) {
					util::displayError("Failed to create fence");
				}

				if (vkCreateSemaphore(mDevice->getDevice(), &semaphoreCreateInfo, nullptr, &mFrames[i].presentSemaphore) != VK_SUCCESS) {
					util::displayError("Failed to create present semaphore");
				}

				if (vkCreateSemaphore(mDevice->getDevice(), &semaphoreCreateInfo, nullptr, &mFrames[i].renderSemaphore) != VK_SUCCESS) {
					util::displayError("Failed to create render semaphore");
				}
			}

			mMainDeletionQueue.pushFunction([=]() {
				for (int i = 0; i < FRAME_OVERLAP; i++) {
					vkDestroyFence(mDevice->getDevice(), mFrames[i].renderFence, nullptr);

					vkDestroySemaphore(mDevice->getDevice(), mFrames[i].presentSemaphore, nullptr);
					vkDestroySemaphore(mDevice->getDevice(), mFrames[i].renderSemaphore, nullptr);
				}
				});
		}


		void Renderer::initDescriptors() {
			// Create descriptor pool
			std::vector<VkDescriptorPoolSize> sizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 }
			};

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.pNext = nullptr;

			poolInfo.flags = 0;
			poolInfo.maxSets = 10;
			poolInfo.poolSizeCount = (uint32_t)sizes.size();
			poolInfo.pPoolSizes = sizes.data();

			vkCreateDescriptorPool(mDevice->getDevice(), &poolInfo, nullptr, &mDescriptorPool);

			// Create descriptor set layout
			VkDescriptorSetLayoutBinding camBind{};
			camBind.binding = 0;
			camBind.descriptorCount = 1;
			camBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			camBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding sceneBind{};
			sceneBind.binding = 1;
			sceneBind.descriptorCount = 1;
			sceneBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			sceneBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding bindings[]{ camBind, sceneBind };

			VkDescriptorSetLayoutCreateInfo setInfo{};
			setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			setInfo.pNext = nullptr;

			setInfo.bindingCount = 2;
			setInfo.pBindings = bindings;

			setInfo.flags = 0;

			vkCreateDescriptorSetLayout(mDevice->getDevice(), &setInfo, nullptr, &mGlobalSetLayout);

			// Create scene parameter buffer
			const size_t sceneParamBufferSize = FRAME_OVERLAP * padUniformBufferSize(sizeof(GPUSceneData));
			mSceneParameterBuffer = createBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			mAllocationDeletionQueue.pushFunction([=]() {
				vmaDestroyBuffer(mAllocator, mSceneParameterBuffer.buffer, mSceneParameterBuffer.allocation);
			});

			// Set up camera buffers
			for (int i = 0; i < FRAME_OVERLAP; i++) {
				mFrames[i].cameraBuffer = createBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Allocate one descriptor set for each frame
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.pNext = nullptr;

				allocInfo.descriptorPool = mDescriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &mGlobalSetLayout;

				vkAllocateDescriptorSets(mDevice->getDevice(), &allocInfo, &mFrames[i].globalDescriptor);

				// Bind buffer to descriptor set
				VkDescriptorBufferInfo cameraBufferInfo{};
				cameraBufferInfo.buffer = mFrames[i].cameraBuffer.buffer;
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(GPUCameraData);

				VkDescriptorBufferInfo sceneBufferInfo{};
				sceneBufferInfo.buffer = mSceneParameterBuffer.buffer;
				sceneBufferInfo.offset = 0;
				sceneBufferInfo.range = sizeof(GPUSceneData);
				


				VkWriteDescriptorSet camWrite{};
				camWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				camWrite.pNext = nullptr;

				// Write to binding #0
				camWrite.dstBinding = 0;

				camWrite.dstSet = mFrames[i].globalDescriptor;

				camWrite.descriptorCount = 1;
				camWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				camWrite.pBufferInfo = &cameraBufferInfo;



				VkWriteDescriptorSet sceneWrite{};
				sceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				sceneWrite.pNext = nullptr;

				// Write to binding #1
				sceneWrite.dstBinding = 1;

				sceneWrite.dstSet = mFrames[i].globalDescriptor;

				sceneWrite.descriptorCount = 1;
				sceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				sceneWrite.pBufferInfo = &sceneBufferInfo;



				VkWriteDescriptorSet setWrites[]{ camWrite, sceneWrite };


				vkUpdateDescriptorSets(mDevice->getDevice(), 2, setWrites, 0, nullptr);

				mAllocationDeletionQueue.pushFunction([=]() {
					vmaDestroyBuffer(mAllocator, mFrames[i].cameraBuffer.buffer, mFrames[i].cameraBuffer.allocation);
				});
			}

			mMainDeletionQueue.pushFunction([=]() {
				vkDestroyDescriptorSetLayout(mDevice->getDevice(), mGlobalSetLayout, nullptr);
				vkDestroyDescriptorPool(mDevice->getDevice(), mDescriptorPool, nullptr);
			});
		}

		void Renderer::createPipelines() {
			// Create shader modules
			VkShaderModule triangleFragShader;
			if (!loadShaderModule("../../shaders/tri.frag.spv", &triangleFragShader)) {
				util::displayMessage("Failed to build the triangle fragment shader module", DISPLAY_TYPE_ERR);
			}
			else {
				util::displayMessage("Triangle fragment shader successfully loaded", DISPLAY_TYPE_INFO);
			}

			VkShaderModule triangleVertexShader;
			if (!loadShaderModule("../../shaders/tri.vert.spv", &triangleVertexShader)) {
				util::displayMessage("Failed to build the triangle vertex shader module", DISPLAY_TYPE_ERR);
			}
			else {
				util::displayMessage("Triangle vertex shader successfully loaded", DISPLAY_TYPE_INFO);
			}


			// Create pipeline layout
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineBuilder::getPipelineLayoutCreateInfo();

			// Setup push constants
			VkPushConstantRange pushConstant;
			pushConstant.offset = 0;
			pushConstant.size = sizeof(MeshPushConstants);
			pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &mGlobalSetLayout;

			if (vkCreatePipelineLayout(mDevice->getDevice(), &pipelineLayoutInfo, nullptr, &mTrianglePipelineLayout) != VK_SUCCESS) {
				util::displayError("Failed to create pipeline layout");
			}


			// Create pipeline object
			PipelineBuilder pipelineBuilder;

			pipelineBuilder.shaderStages.push_back(PipelineBuilder::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
			pipelineBuilder.shaderStages.push_back(PipelineBuilder::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

			VertexInputDescription vertexInputDescription = Vertex::getVertexDescription();


			VkPipelineVertexInputStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.vertexAttributeDescriptionCount = vertexInputDescription.attributes.size();
			info.pVertexAttributeDescriptions = vertexInputDescription.attributes.data();

			info.vertexBindingDescriptionCount = vertexInputDescription.bindings.size();
			info.pVertexBindingDescriptions = vertexInputDescription.bindings.data();

			pipelineBuilder.vertexInputInfo = info;

			pipelineBuilder.inputAssembly = PipelineBuilder::getInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			// Build viewport and scissor
			VkExtent2D windowExtent = mWindow->getExtent();

			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)windowExtent.width;
			pipelineBuilder.viewport.height = (float)windowExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = windowExtent;

			pipelineBuilder.rasterizer = PipelineBuilder::getRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

			pipelineBuilder.multisampling = PipelineBuilder::getMultisamplingStateCreateInfo();

			pipelineBuilder.colorBlendAttachment = PipelineBuilder::getColorBlendAttachmentState();

			pipelineBuilder.depthStencil = PipelineBuilder::getDepthStencilState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			pipelineBuilder.pipelineLayout = mTrianglePipelineLayout;

			mTrianglePipeline = pipelineBuilder.buildPipeline(mDevice->getDevice(), mRenderPass);

			createMaterial(mTrianglePipeline, mTrianglePipelineLayout, "defaultmesh");

			vkDestroyShaderModule(mDevice->getDevice(), triangleFragShader, nullptr);
			vkDestroyShaderModule(mDevice->getDevice(), triangleVertexShader, nullptr);

			mMainDeletionQueue.pushFunction([&]() {
				vkDestroyPipeline(mDevice->getDevice(), mTrianglePipeline, nullptr);

				vkDestroyPipelineLayout(mDevice->getDevice(), mTrianglePipelineLayout, nullptr);
			});
		}


		void Renderer::loadMeshes() {
			mTriangleMesh.vertices.resize(3);

			mTriangleMesh.vertices[0].position = { 1.f,1.f, 0.5f };
			mTriangleMesh.vertices[1].position = { -1.f,1.f, 0.5f };
			mTriangleMesh.vertices[2].position = { 0.f,-1.f, 0.5f };

			mTriangleMesh.vertices[0].color = { 0.f,1.f, 0.0f }; //pure green
			mTriangleMesh.vertices[1].color = { 0.f,1.f, 0.0f }; //pure green
			mTriangleMesh.vertices[2].color = { 0.f,1.f, 0.0f }; //pure green

			mMesh.loadFromObj("../../assets/monkey_smooth.obj");

			uploadMesh(mTriangleMesh);
			uploadMesh(mMesh);
			
			mMeshes["monkey"] = mMesh;
			mMeshes["triangle"] = mTriangleMesh;
		}

		void Renderer::initScene() {
			RenderObject monkey;
			monkey.mesh = getMesh("monkey");
			monkey.material = getMaterial("defaultmesh");
			monkey.transformMatrix = glm::mat4{ 1.0f };

			mRenderables.push_back(monkey);

			for (int x = -50; x <= 50; x++) {
				for (int y = -50; y <= 50; y++) {
					RenderObject tri;
					tri.mesh = getMesh("monkey");
					tri.material = getMaterial("defaultmesh");
					glm::mat4 translation = glm::translate(glm::mat4{1.0f}, glm::vec3(x, 0, y));
					glm::mat4 scale = glm::scale(glm::mat4{1.0f}, glm::vec3(0.2f, 0.2f, 0.2f));
					tri.transformMatrix = translation * scale;

					mRenderables.push_back(tri);
				}
			}
		}


		// Helper functions
		bool Renderer::checkValidationLayerSupport() const {
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : VALIDATION_LAYERS) {
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers) {
					if (strcmp(layerName, layerProperties.layerName) == 0) {
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
					return false;
			}

			return true;
		}

		bool Renderer::checkInstanceExtensionSupport(std::vector<const char*>& extensions) const {
			uint32_t supportedExtensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);

			std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);

			vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

			for (uint32_t i = 0; i < extensions.size(); i++) {
				bool hasExtension = false;

				for (const auto& extension : supportedExtensions) {
					if (strcmp(extension.extensionName, extensions[i]) == 0) {
						hasExtension = true;
						break;
					}
				}

				if (!hasExtension)
					return false;
			}

			return true;
		}

		size_t Renderer::padUniformBufferSize(size_t originalSize) const {
			size_t minUboAlignment = mDevice->getDeviceProperties().gpuProperties.limits.minUniformBufferOffsetAlignment;
			size_t alignedSize = originalSize;

			if (minUboAlignment > 0) {
				alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			return alignedSize;
		}

		bool Renderer::loadShaderModule(const char* filePath, VkShaderModule* outShaderModule) const {
			// Open the file in binary with the cursor at the end
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				return false;
			}

			// Find the size of the file by looking up the location of the cursor
			// Since the cursor starts at the end, it gives the size directly in bytes
			size_t fileSize = (size_t)file.tellg();

			// Spirv expects the buffer to be on uint32
			std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

			// Put file cursor at beginning
			file.seekg(0);

			// Load entire file into the buffer
			file.read((char*)buffer.data(), fileSize);

			file.close();


			// Now create shader module
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;

			createInfo.codeSize = buffer.size() * sizeof(uint32_t);
			createInfo.pCode = buffer.data();

			VkShaderModule shaderModule;

			if (vkCreateShaderModule(mDevice->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				return false;
			}

			*outShaderModule = shaderModule;

			return true;
		}

		void Renderer::uploadMesh(Mesh& mesh) {
			// Allocate vertex buffer
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = nullptr;

			bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
			bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

			VmaAllocationCreateInfo vmaAllocInfo{};
			vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

			if (vmaCreateBuffer(mAllocator, &bufferInfo, &vmaAllocInfo,
				&mesh.vertexBuffer.buffer,
				&mesh.vertexBuffer.allocation,
				nullptr) != VK_SUCCESS) {
				util::displayError("Failed to create vertex buffer");
			}

			mAllocationDeletionQueue.pushFunction([=]() {
				vmaDestroyBuffer(mAllocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
			});

			// Copy vertex data
			void* data;
			vmaMapMemory(mAllocator, mesh.vertexBuffer.allocation, &data);

			memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));

			vmaUnmapMemory(mAllocator, mesh.vertexBuffer.allocation);
		}

		Mesh* Renderer::getMesh(const std::string& name) {
			auto it = mMeshes.find(name);

			if (it == mMeshes.end()) {
				return nullptr;
			}
			else {
				return &it->second;
			}
		}

		Material* Renderer::createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name) {
			Material mat;
			mat.pipeline = pipeline;
			mat.pipelineLayout = layout;
			mMaterials[name] = mat;
			return &mMaterials[name];
		}

		Material* Renderer::getMaterial(const std::string& name) {
			auto it = mMaterials.find(name);
			
			if (it == mMaterials.end()) {
				return nullptr;
			}
			else {
				return &it->second;
			}
		}

		AllocatedBuffer Renderer::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = nullptr;

			bufferInfo.size = allocSize;
			bufferInfo.usage = usage;

			VmaAllocationCreateInfo vmaAllocInfo{};
			vmaAllocInfo.usage = memUsage;

			AllocatedBuffer newBuffer;

			if (vmaCreateBuffer(mAllocator, &bufferInfo, &vmaAllocInfo,
				&newBuffer.buffer,
				&newBuffer.allocation,
				nullptr) != VK_SUCCESS) {
				util::displayError("Failed to create buffer");
			}

			return newBuffer;
		}


		void Renderer::cleanupSwapchain() {
			for (auto framebuffer : mSwapchainFramebuffers) {
				vkDestroyFramebuffer(mDevice->getDevice(), framebuffer, nullptr);
			}

			for (auto imageView : mSwapchainImageViews) {
				vkDestroyImageView(mDevice->getDevice(), imageView, nullptr);
			}

			vkDestroySwapchainKHR(mDevice->getDevice(), mSwapchain, nullptr);
		}
	}
}