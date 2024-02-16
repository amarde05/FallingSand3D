#pragma once

#include <vulkan/vulkan.h>

namespace engine {
	namespace rendering {
		namespace tools {
			VkWriteDescriptorSet createDescriptorSetWrite(VkDescriptorType type, VkDescriptorSet set, uint32_t binding);

			VkDescriptorSetLayoutCreateInfo createDescriptorSetLayoutInfo(uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings, VkDescriptorSetLayoutCreateFlags flags);
		}
	}
}