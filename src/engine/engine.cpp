#include "engine.h"
#include "./util/debug.h"

#include <SDL_vulkan.h>

#include <chrono>
#include <thread>
#include <algorithm>
#include <array>

namespace {
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

		std::string msg = "Validation layer: ";
		msg += pCallbackData->pMessage;

		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			util::displayMessage(msg, WARN);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			util::displayMessage(msg, ERR);
			break;
		default:
			util::displayMessage(msg, INFO);
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
	const std::vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

	VulkanEngine* loadedEngine = nullptr;

	VulkanEngine& VulkanEngine::Get() {
		return *loadedEngine;
	}

	void VulkanEngine::init() {
		// Ensure there is only one instance of VulkanEngine
		if (loadedEngine != nullptr)
			util::displayError("Can't have more than one engine.");

		loadedEngine = this;

		// Initialize SDL
		SDL_Init(SDL_INIT_VIDEO);

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

		mWindow = SDL_CreateWindow(
			mApplicationName,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			mWindowExtent.width,
			mWindowExtent.height,
			window_flags
		);

		// Call initialization functions
		createInstance();
		setupDebugMessenger();
		createSurface();

		mDevice = std::make_unique<VulkanDevice>(mInstance, mSurface, ENABLE_VALIDATION_LAYERS, VALIDATION_LAYERS);

		createSwapchain();
		createSwapchainImageViews();

		allocateCommandBuffers();

		createRenderPass();
		createFramebuffers();

		createSyncStructures();

		// Everything is initialized, so set mIsInitialized to true
		mIsInitialized = true;
	}

	void VulkanEngine::cleanup() {
		if (mIsInitialized) {
			cleanupSwapchain();

			vkDestroyRenderPass(mDevice->getDevice(), mRenderPass, nullptr);

			for (int i = 0; i < FRAME_OVERLAP; i++) {
				vkDestroyFence(mDevice->getDevice(), mFrames[i].renderFence, nullptr);

				vkDestroySemaphore(mDevice->getDevice(), mFrames[i].presentSemaphore, nullptr);
				vkDestroySemaphore(mDevice->getDevice(), mFrames[i].renderSemaphore, nullptr);
			}

			mDevice = nullptr;

			vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

			if (ENABLE_VALIDATION_LAYERS) {
				DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
			}

			vkDestroyInstance(mInstance, nullptr);

			SDL_DestroyWindow(mWindow);
		}

		// Engine is no longer loaded, so remove pointer
		loadedEngine = nullptr;
	}

	void VulkanEngine::draw() {
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

		// Make a clear color from frame number. This will flash with a 120*pi frame period
		VkClearValue clearValue;
		float flash = abs(sin(mFrameNumber / 120.0f));
		clearValue.color = { {0.0f, 0.0f, flash, 1.0f } };

		// Start the main render pass
		VkRenderPassBeginInfo renderpassInfo{};
		renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassInfo.pNext = nullptr;

		renderpassInfo.renderPass = mRenderPass;
		renderpassInfo.renderArea.offset.x = 0;
		renderpassInfo.renderArea.offset.y = 0;
		renderpassInfo.renderArea.extent = mWindowExtent;
		renderpassInfo.framebuffer = mSwapchainFramebuffers[swapchainImageIndex];

		// Connect clear values
		renderpassInfo.clearValueCount = 1;
		renderpassInfo.pClearValues = &clearValue;

		// Begin the render pass
		vkCmdBeginRenderPass(cmd, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

	void VulkanEngine::run() {
		if (!mIsInitialized)
			return;

		SDL_Event e;
		bool bQuit = false;

		// Main loop
		while (!bQuit) {
			// Handle SDL events
			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT) {
					bQuit = true;
				}
				else if (e.type == SDL_WINDOWEVENT) {
					if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
						mStopRendering = true;
					}
					else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
						mStopRendering = false;
					}
				}
			}

			// Don't draw if minimized
			if (mStopRendering) {
				// Throttle speed to avoid endless spinning
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			else {
				draw();
			}
		}

		// Ensure that no more graphics commands are being run
		vkQueueWaitIdle(mDevice->getGraphicsQueue());
	}

	// Vulkan initialization functions
	void VulkanEngine::createInstance() {
		if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
			util::displayError("Could not create vulkan instance because required validation layers are not supported");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = mApplicationName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = ENGINE_NAME;
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		std::vector<const char*> requiredExtensions = getRequiredSDLExtensions();

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

	void VulkanEngine::setupDebugMessenger() {
		if (!ENABLE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
			util::displayError("Failed to create debug messenger");
		}
	}


	void VulkanEngine::createSurface() {
		if (SDL_Vulkan_CreateSurface(mWindow, mInstance, &mSurface) != SDL_TRUE) {
			util::displayError("Failed to create surface!");
		}
	}

	void VulkanEngine::createSwapchain() {
		SwapchainSupportDetails swapChainSupport = mDevice->getSwapchainSupport();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, mWindow);

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
	}

	void VulkanEngine::createSwapchainImageViews() {
		mSwapchainImageViews.resize(mSwapchainImages.size());

		for (size_t i = 0; i < mSwapchainImages.size(); i++) {
			mDevice->createImageView(mSwapchainImages[i], mSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, mSwapchainImageViews[i]);
		}
	}


	void VulkanEngine::allocateCommandBuffers() {
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			mDevice->getGraphicsPool().allocateBuffers(&mFrames[i].frameCommandBuffer, 1);
		}
	}


	void VulkanEngine::createRenderPass() {
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
		depthAttachment.format = findDepthFormat(mDevice->getPhysicalDevice());
		depthAttachment.samples = mDevice->getDeviceProperties().maxSamples;
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
		//subpass.pDepthStencilAttachment = &depthAttachmentRef;

		/*VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		*/
		//std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;//static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = &colorAttachment;//attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		//renderPassInfo.dependencyCount = 1;
		//renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(mDevice->getDevice(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
			util::displayError("Failed to create renderpass");
		}
	}

	void VulkanEngine::createFramebuffers() {
		// Create framebuffers for the swapchain images
		// This will connect the renderpass to the images for rendering
		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.pNext = nullptr;

		frameBufferInfo.renderPass = mRenderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.width = mWindowExtent.width;
		frameBufferInfo.height = mWindowExtent.height;
		frameBufferInfo.layers = 1;

		const uint32_t swapchainImageCount = mSwapchainImages.size();
		mSwapchainFramebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

		// Create framebuffers for each of the swapchain image views
		for (int i = 0; i < swapchainImageCount; i++) {
			frameBufferInfo.pAttachments = &mSwapchainImageViews[i];

			if (vkCreateFramebuffer(mDevice->getDevice(), &frameBufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS) {
				util::displayError("Failed to create framebuffers");
			}
		}
	}


	void VulkanEngine::createSyncStructures() {
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
	}

	void VulkanEngine::createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(mDevice->getDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
			util::displayError("Failed to create descriptor set layout");
		}
	}	


	// Helper functions
	bool VulkanEngine::checkValidationLayerSupport() const {
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

	bool VulkanEngine::checkInstanceExtensionSupport(std::vector<const char*>& extensions) const {
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

	std::vector<const char*> VulkanEngine::getRequiredSDLExtensions() const {
		unsigned int count;

		SDL_Vulkan_GetInstanceExtensions(mWindow, &count, NULL);

		std::vector<const char*> requiredExtensions(count);

		SDL_Vulkan_GetInstanceExtensions(mWindow, &count, requiredExtensions.data());

		return requiredExtensions;
	}
	
	void VulkanEngine::cleanupSwapchain() {
		for (auto framebuffer : mSwapchainFramebuffers) {
			vkDestroyFramebuffer(mDevice->getDevice(), framebuffer, nullptr);
		}

		for (auto imageView : mSwapchainImageViews) {
			vkDestroyImageView(mDevice->getDevice(), imageView, nullptr);
		}

		vkDestroySwapchainKHR(mDevice->getDevice(), mSwapchain, nullptr);
	}
}