#pragma once

#include "../device.h"
#include "../deletion_queue.h"

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

namespace engine {
	namespace rendering {
		namespace memory {
			struct AllocatedBuffer {
				VkBuffer buffer;
				VmaAllocation allocation;
			};

			struct AllocatedImage {
				VkImage image;
				VmaAllocation allocation;
			};

			VmaAllocator& getAllocator();

			DeletionQueue& getAllocationDeletionQueue();


			AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);



			void createAllocator(VkInstance instance, VulkanDevice* device);

			void cleanupAllocations();
		}
	}
}