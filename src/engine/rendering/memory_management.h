#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

namespace engine {
	namespace rendering {
		struct AllocatedBuffer {
			VkBuffer buffer;
			VmaAllocation allocation;
		};

		struct AllocatedImage {
			VkImage image;
			VmaAllocation allocation;
		};


	}
}