#include "initializers.h"

namespace engine {
	namespace rendering {
		namespace tools {
			VkWriteDescriptorSet createDescriptorSetWrite(VkDescriptorType type, VkDescriptorSet set, uint32_t binding) {
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.pNext = nullptr;

				write.dstBinding = binding;

				write.dstSet = set;

				write.descriptorCount = 1;
				write.descriptorType = type;

				return write;
			}

			VkDescriptorSetLayoutCreateInfo createDescriptorSetLayoutInfo(uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings, VkDescriptorSetLayoutCreateFlags flags) {
				VkDescriptorSetLayoutCreateInfo setInfo{};
				setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				setInfo.pNext = nullptr;

				setInfo.bindingCount = bindingCount;
				setInfo.pBindings = pBindings;

				setInfo.flags = flags;

				return setInfo;
			}

			VkFenceCreateInfo createFence(VkFenceCreateFlags flags) {
				VkFenceCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				info.pNext = nullptr;
				info.flags = flags;

				return info;
			}

			VkSemaphoreCreateInfo createSemaphore(VkSemaphoreCreateFlags flags) {
				VkSemaphoreCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				info.pNext = nullptr;
				info.flags = flags;

				return info;
			}

			VkCommandBufferBeginInfo createCommandBufferBeginInfo(VkCommandBufferUsageFlags usageFlags) {
				VkCommandBufferBeginInfo info{};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				info.pNext = nullptr;

				info.pInheritanceInfo = nullptr;
				info.flags = usageFlags;

				return info;
			}

			VkSubmitInfo createSubmitInfo(VkCommandBuffer* cmd) {
				VkSubmitInfo info{};
				info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				info.pNext = nullptr;

				info.waitSemaphoreCount = 0;
				info.pWaitSemaphores = nullptr;
				info.pWaitDstStageMask = nullptr;
				info.commandBufferCount = 1;
				info.pCommandBuffers = cmd;
				info.signalSemaphoreCount = 0;
				info.pSignalSemaphores = nullptr;

				return info;
			}

			VkCommandPoolCreateInfo createCommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.pNext = nullptr;

				poolInfo.queueFamilyIndex = queueFamilyIndex;
				poolInfo.flags = flags;

				return poolInfo;
			}
		
			VkImageCreateInfo createImageInfo(VkFormat imageFormat, VkImageUsageFlags flags, VkExtent3D extent) {
				VkImageCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				info.pNext = nullptr;

				info.imageType = VK_IMAGE_TYPE_2D;

				info.format = imageFormat;
				info.extent = extent;

				info.mipLevels = 1;
				info.arrayLayers = 1;

				info.samples = VK_SAMPLE_COUNT_1_BIT;

				info.tiling = VK_IMAGE_TILING_OPTIMAL;
				info.usage = flags;
				
				return info;
			}

			VkImageViewCreateInfo createImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
				VkImageViewCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				info.pNext = nullptr;

				info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				info.image = image;
				info.format = format;
				info.subresourceRange.baseMipLevel = 0;
				info.subresourceRange.levelCount = 1;
				info.subresourceRange.baseArrayLayer = 0;
				info.subresourceRange.layerCount = 1;
				info.subresourceRange.aspectMask = aspectFlags;

				return info;
			}

			VkSamplerCreateInfo createSamplerInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode) {
				VkSamplerCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				info.pNext = nullptr;
				
				info.magFilter = filters;
				info.minFilter = filters;
				info.addressModeU = samplerAddressMode;
				info.addressModeV = samplerAddressMode;
				info.addressModeW = samplerAddressMode;
				
				return info;
			}

			VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding) {
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.pNext = nullptr;

				write.dstBinding = binding;
				write.dstSet = dstSet;
				write.descriptorCount = 1;
				write.descriptorType = type;
				write.pImageInfo = imageInfo;

				return write;
			}
		}
	}
}