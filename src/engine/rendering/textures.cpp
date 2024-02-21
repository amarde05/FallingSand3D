#include "textures.h"

#include "../../util/debug.h"

#include "tools/initializers.h"

#include <vulkan/vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace engine {
	namespace rendering {
		bool loadImageFromFile(Renderer& renderer, const char* file, AllocatedImage& outImage) {
			int texWidth, texHeight, texChannels;

			stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!pixels) {
				util::displayMessage("Failed to load texture file: " + std::string(file), DISPLAY_TYPE_WARN);
				return false;
			}

			VmaAllocator allocator = renderer.getAllocator();

			//void* pixelData = pixels;
			VkDeviceSize imageSize = texWidth * texHeight * 4;

			// Format that matches the pixels loaded from stb_image lib
			VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

			// Allocate temporary buffer for holding texture data to upload
			AllocatedBuffer stagingBuffer = renderer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Copy data to buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);

			memcpy(data, pixels, static_cast<size_t>(imageSize));

			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			stbi_image_free(pixels);

			VkExtent3D imageExtent;
			imageExtent.width = static_cast<uint32_t>(texWidth);
			imageExtent.height = static_cast<uint32_t>(texHeight);
			imageExtent.depth = 1;

			VkImageCreateInfo dimgInfo = tools::createImageInfo(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

			AllocatedImage newImage;

			VmaAllocationCreateInfo dimgAllocInfo{};
			dimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			vmaCreateImage(allocator, &dimgInfo, &dimgAllocInfo, &newImage.image, &newImage.allocation, nullptr);

			renderer.immediateSubmit([&](VkCommandBuffer cmd) {
				VkImageSubresourceRange range;
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel = 0;
				range.levelCount = 1;
				range.baseArrayLayer = 0;
				range.layerCount = 1;

				// Create a pipeline barrier
				VkImageMemoryBarrier imageBarrierToTransfer{};
				imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarrierToTransfer.pNext = nullptr;

				imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrierToTransfer.image = newImage.image;
				imageBarrierToTransfer.subresourceRange = range;

				imageBarrierToTransfer.srcAccessMask = 0;
				imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);
				
				// Copy image data
				VkBufferImageCopy copyRegion{};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = imageExtent;

				vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
			
				// Make data in shader-readable format
				VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
				imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
			});

			renderer.getAllocationDeletionQueue().pushFunction([=]() {
				vmaDestroyImage(allocator, newImage.image, newImage.allocation);
			});

			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

			util::displayMessage("Texture loaded successfully: " + std::string(file), DISPLAY_TYPE_INFO);

			outImage = newImage;
			return true;
		}
	}
}