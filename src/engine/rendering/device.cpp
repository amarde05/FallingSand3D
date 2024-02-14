#include "device.h"
#include "../../util/debug.h"

#include <string>
#include <set>
#include <map>

namespace engine {
	namespace rendering {
		VulkanDevice::VulkanDevice(VkInstance& instance, VkSurfaceKHR& surface,
			const bool validationLayersEnabled, const std::vector<const char*>& validationLayers) : rSurface{ surface } {
			pickPhysicalDevice(instance);
			createLogicalDevice(validationLayersEnabled, validationLayers);

			mGraphicsPool = std::make_unique<VulkanCommandPool>(mQueueFamilyIndices.graphicsFamily, mGraphicsQueue, mDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			mTransferPool = std::make_unique<VulkanCommandPool>(mQueueFamilyIndices.transferFamily, mTransferQueue, mDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		}

		VulkanDevice::~VulkanDevice() {
			mGraphicsPool = nullptr;
			mTransferPool = nullptr;
			vkDestroyDevice(mDevice, nullptr);
		}


		uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			util::displayError("Failed to find a suitable memory type!");
		}

		VkFormat VulkanDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
			for (VkFormat format : candidates) {
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					return format;
				}
			}

			util::displayError("Failed to find a supported format");
		}


		void VulkanDevice::createBuffer(VkDeviceSize allocSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
			VkBuffer& buffer, VkDeviceMemory& bufferMemory) const {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = allocSize;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
				util::displayError("Failed to create buffer");
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

			if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
				util::displayError("Failed to allocate buffer memory");
			}

			vkBindBufferMemory(mDevice, buffer, bufferMemory, 0);
		}



		void VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const {
			VkCommandBuffer commandBuffer = mTransferPool->beginSingleTimeCommands();

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			mTransferPool->endSingleTimeCommands(commandBuffer);
		}

		void VulkanDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) const {
			VkCommandBuffer commandBuffer = mTransferPool->beginSingleTimeCommands();

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = layerCount;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			mTransferPool->endSingleTimeCommands(commandBuffer);
		}


		void VulkanDevice::createImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const {
			if (vkCreateImage(mDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
				util::displayError("Failed to create image");
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(mDevice, image, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

			if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
				util::displayError("Failed to allocate image memory");
			}

			if (vkBindImageMemory(mDevice, image, imageMemory, 0) != VK_SUCCESS) {
				util::displayError("Failed to bind image memory");
			}
		}

		void VulkanDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView& imageView) const {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(mDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
				util::displayError("Failed to create image view");
			}
		}



		void VulkanDevice::pickPhysicalDevice(VkInstance& instance) {
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0) {
				util::displayError("Could not select a physical device because failed to find GPUs with Vulkan support!");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			std::multimap<int, VkPhysicalDevice> candidates;

			for (const auto& device : devices) {
				int score = rateDeviceSuitability(device);
				candidates.insert(std::make_pair(score, device));
			}

			if (candidates.rbegin()->first > 0) {
				mPhysicalDevice = candidates.rbegin()->second;
				mDeviceProperties.maxSamples = getMaxUsableSampleCount(mPhysicalDevice);
				
				vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties.gpuProperties);

				mQueueFamilyIndices = findPhysicalQueueFamilies();
			}
			else {
				util::displayError("Could not select a physical device because failed to find a suitable GPU");
			}
		}

		void VulkanDevice::createLogicalDevice(const bool validationLayersEnabled, const std::vector<const char*>& validationLayers) {
			QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

			float queuePriority = 1.0f;

			for (uint32_t queueFamily : uniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.samplerAnisotropy = VK_TRUE;
			deviceFeatures.sampleRateShading = VK_TRUE;

			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();

			createInfo.pEnabledFeatures = &deviceFeatures;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
			createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

			if (validationLayersEnabled) {
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else {
				createInfo.enabledLayerCount = 0;
			}

			if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS) {
				util::displayError("Failed to create logical device");
			}

			// Set queue handles
			vkGetDeviceQueue(mDevice, indices.graphicsFamily, 0, &mGraphicsQueue);
			vkGetDeviceQueue(mDevice, indices.presentFamily, 0, &mPresentQueue);
			vkGetDeviceQueue(mDevice, indices.transferFamily, 0, &mTransferQueue);
		}

		int VulkanDevice::rateDeviceSuitability(VkPhysicalDevice device) const {
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			int score = 0;

			// Discrete GPUs have a significant performance advantage
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			// Maximum possible size of textures affects graphics quality
			score += deviceProperties.limits.maxImageDimension2D;

			// Application can't function without geometry shaders
			if (!deviceFeatures.geometryShader) {
				return 0;
			}

			QueueFamilyIndices indices = findQueueFamilies(device);
			if (!indices.isComplete()) {
				return 0;
			}

			// Having different queues that handle seperate things can lead to better performance
			if (indices.graphicsFamily != indices.presentFamily) {
				score += 1000;
			}

			bool extensionsSupported = checkDeviceExtensionSupport(device);
			if (!extensionsSupported) {
				return 0;
			}
			else {
				SwapchainSupportDetails swapChainSupport = querySwapchainSupport(device);
				bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

				if (!swapChainAdequate) {
					return 0;
				}
			}

			if (!deviceFeatures.samplerAnisotropy) {
				return 0;
			}

			return score;
		}

		VkSampleCountFlagBits VulkanDevice::getMaxUsableSampleCount(VkPhysicalDevice physicalDevice) const {
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
			if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; };
			if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; };
			if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; };
			if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; };
			if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; };
			if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; };

			return VK_SAMPLE_COUNT_1_BIT;
		}

		QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) const {
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			int i = 0;

			for (const auto& queueFamily : queueFamilies) {
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily = i;
					indices.graphicsFamilyHasValue = true;
				}

				if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
					indices.transferFamily = i;
					indices.transferFamilyHasValue = true;
				}

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, rSurface, &presentSupport);

				if (presentSupport) {
					indices.presentFamily = i;
					indices.presentFamilyHasValue = true;
				}

				if (indices.isComplete()) {
					break;
				}

				i++;
			}

			return indices;
		}

		SwapchainSupportDetails VulkanDevice::querySwapchainSupport(VkPhysicalDevice device) const {
			SwapchainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, rSurface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, rSurface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, rSurface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, rSurface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, rSurface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) const {
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

			for (const auto& extension : availableExtensions) {
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}
	}
}