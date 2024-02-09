#include "commands.h"
#include "./util/debug.h"


namespace engine {
	VulkanCommandPool::VulkanCommandPool(uint32_t queueFamilyIndex, VkQueue& queue, VkDevice& device, VkCommandPoolCreateFlags createFlags) : rQueue{ queue }, rDevice{ device } {
		mQueueFamilyIndex = queueFamilyIndex;

		createCommandPool(createFlags);
	}

	VulkanCommandPool::~VulkanCommandPool() {
		vkDestroyCommandPool(rDevice, mCommandPool, nullptr);
	}

	void VulkanCommandPool::cleanup() {
		vkDestroyCommandPool(rDevice, mCommandPool, nullptr);
	}

	VkCommandBuffer VulkanCommandPool::beginSingleTimeCommands() const {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = mCommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(rDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void VulkanCommandPool::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(rQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(rQueue);

		vkFreeCommandBuffers(rDevice, mCommandPool, 1, &commandBuffer);
	}

	void VulkanCommandPool::allocateBuffers(VkCommandBuffer* buffers, int count) const {
		VkCommandBufferAllocateInfo cmdAllocInfo{};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = mCommandPool;
		cmdAllocInfo.commandBufferCount = count;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vkAllocateCommandBuffers(rDevice, &cmdAllocInfo, buffers);
	}

	void VulkanCommandPool::createCommandPool(VkCommandPoolCreateFlags createFlags) {
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.queueFamilyIndex = mQueueFamilyIndex;
		poolInfo.flags = createFlags;

		if (vkCreateCommandPool(rDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
			util::displayError("Failed to create command pool");
		}
	}
}