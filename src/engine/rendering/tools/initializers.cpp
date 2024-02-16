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
		}
	}
}