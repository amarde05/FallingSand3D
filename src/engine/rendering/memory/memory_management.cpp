#include "memory_management.h"
#include "../../../util/debug.h"

namespace engine {
	namespace rendering {
		namespace memory {
			DeletionQueue allocationDeletionQueue;

			VmaAllocator allocator;


			AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {
				VkBufferCreateInfo bufferInfo{};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;

				bufferInfo.size = allocSize;
				bufferInfo.usage = usage;

				VmaAllocationCreateInfo vmaAllocInfo{};
				vmaAllocInfo.usage = memUsage;

				memory::AllocatedBuffer newBuffer;

				if (vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo,
					&newBuffer.buffer,
					&newBuffer.allocation,
					nullptr) != VK_SUCCESS) {
					util::displayError("Failed to create buffer");
				}

				return newBuffer;
			}



			VmaAllocator& getAllocator() {
				return allocator;
			}

			DeletionQueue& getAllocationDeletionQueue() {
				return allocationDeletionQueue;
			}

			void createAllocator(VkInstance instance, VulkanDevice* device) {
				VmaAllocatorCreateInfo allocatorInfo{};

				allocatorInfo.physicalDevice = device->getPhysicalDevice();
				allocatorInfo.device = device->getDevice();
				allocatorInfo.instance = instance;

				vmaCreateAllocator(&allocatorInfo, &allocator);
			}

			void cleanupAllocations() {
				allocationDeletionQueue.flush();

				vmaDestroyAllocator(allocator);
			}
		}
	}
}