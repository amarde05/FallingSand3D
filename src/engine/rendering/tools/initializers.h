#pragma once

#include <vulkan/vulkan.h>

namespace engine {
	namespace rendering {
		namespace tools {
			VkDescriptorSetLayoutCreateInfo createDescriptorSetLayoutInfo(uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings, VkDescriptorSetLayoutCreateFlags flags);
			VkWriteDescriptorSet createDescriptorSetWrite(VkDescriptorType type, VkDescriptorSet set, uint32_t binding);
			VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
			VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
			VkSamplerCreateInfo createSamplerInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
			
			VkFenceCreateInfo createFence(VkFenceCreateFlags flags = 0);
			VkSemaphoreCreateInfo createSemaphore(VkSemaphoreCreateFlags flags = 0);

			VkCommandBufferBeginInfo createCommandBufferBeginInfo(VkCommandBufferUsageFlags usageFlags = 0);
			VkSubmitInfo createSubmitInfo(VkCommandBuffer* cmd);
			VkCommandPoolCreateInfo createCommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
		
			VkImageCreateInfo createImageInfo(VkFormat imageFormat, VkImageUsageFlags flags, VkExtent3D extent);
			VkImageViewCreateInfo createImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
		
			
		}
	}
}