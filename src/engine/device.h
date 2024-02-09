#pragma once

#include "commands.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

namespace engine {
	const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t presentFamily;
		uint32_t transferFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool transferFamilyHasValue = false;
		bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue && transferFamilyHasValue; }
	};

	struct DeviceProperties {
		VkSampleCountFlagBits maxSamples;
	};

	class VulkanDevice {
	public:
		VulkanDevice(VkInstance& instance, VkSurfaceKHR& surface, const bool validationLayersEnabled, const std::vector<const char*>& validationLayers);
		~VulkanDevice();

		// Disable copying and moving
		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;
		VulkanDevice(VulkanDevice&&) = delete;
		VulkanDevice& operator=(VulkanDevice&&) = delete;

		VulkanCommandPool& getGraphicsPool() const { return *mGraphicsPool.get(); }
		VulkanCommandPool& getTransferPool() const { return *mTransferPool.get(); }
		VkDevice getDevice() const { return mDevice; }
		VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
		VkQueue getGraphicsQueue() const { return mGraphicsQueue; }
		VkQueue getPresentQueue() const { return mPresentQueue; }
		VkQueue getTransferQueue() const { return mTransferQueue; }

		DeviceProperties getDeviceProperties() const { return mDeviceProperties; }
		QueueFamilyIndices getQueueFamilyIndices() const { return mQueueFamilyIndices; }

		SwapchainSupportDetails getSwapchainSupport() const { return querySwapchainSupport(mPhysicalDevice); }
		QueueFamilyIndices findPhysicalQueueFamilies() const { return findQueueFamilies(mPhysicalDevice); }

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) const;

		void createImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
		void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView& imageView) const;
	private:
		VkPhysicalDevice mPhysicalDevice{ VK_NULL_HANDLE };
		// TODO: allow for up to 3 queues: 1 for graphics, 1 for compute and 1 for transfer
		std::unique_ptr<VulkanCommandPool> mGraphicsPool;
		std::unique_ptr<VulkanCommandPool> mTransferPool;

		VkDevice mDevice;
		VkSurfaceKHR& rSurface;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
		VkQueue mTransferQueue;

		DeviceProperties mDeviceProperties;
		QueueFamilyIndices mQueueFamilyIndices;

		void pickPhysicalDevice(VkInstance& instance);
		void createLogicalDevice(const bool validationLayersEnabled, const std::vector<const char*>& validationLayers);

		// Helper functions
		int rateDeviceSuitability(VkPhysicalDevice physicalDevice) const;
		VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice) const;
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
		SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;
		bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
	};
}