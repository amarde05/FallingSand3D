#pragma once

#include <vulkan/vulkan.h>

namespace engine {
	class VulkanCommandPool {
	public:
		VulkanCommandPool(uint32_t queueFamilyIndex, VkQueue& queue, VkDevice& device, VkCommandPoolCreateFlags createFlags);
		~VulkanCommandPool();

		VkCommandPool getPool() { return mCommandPool; }

		VkCommandBuffer beginSingleTimeCommands() const;
		void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

		void allocateBuffers(VkCommandBuffer* buffer, int count) const;

		void cleanup();
	private:
		VkCommandPool mCommandPool;
		uint32_t mQueueFamilyIndex;
		VkQueue& rQueue;
		VkDevice& rDevice;

		void createCommandPool(VkCommandPoolCreateFlags createFlags);
	};
}