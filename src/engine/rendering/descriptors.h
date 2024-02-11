#pragma once

#include "device.h"

#include <memory>
#include <unordered_map>

namespace engine {
	namespace rendering {
		class VulkanDescriptorSetLayout {
		public:
			class Builder {
			public:
				Builder(VulkanDevice& device) : rDevice{ device } {}

				Builder& addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);

				std::unique_ptr<VulkanDescriptorSetLayout> build() const;
			private:
				VulkanDevice& rDevice;
				std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> mBindings{};
			};

			VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
			~VulkanDescriptorSetLayout();

			VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
			VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

			VkDescriptorSetLayout getDescriptorSetLayout() const { return mDescriptorSetLayout; }

		private:
			VulkanDevice& rDevice;
			VkDescriptorSetLayout mDescriptorSetLayout;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> mBindings;

			friend class VulkanDescriptorWriter;
		};

		class VulkanDescriptorPool {
		public:
			class Builder {
			public:
				Builder(VulkanDevice& device) : rDevice{ device } {}

				Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
				Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
				Builder& setMaxSets(uint32_t count);

				std::unique_ptr<VulkanDescriptorPool> build() const;
			private:
				VulkanDevice& rDevice;
				std::vector<VkDescriptorPoolSize> mPoolSizes{};
				uint32_t mMaxSets = 1000;
				VkDescriptorPoolCreateFlags mPoolFlags;
			};

			VulkanDescriptorPool(VulkanDevice& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
			~VulkanDescriptorPool();

			VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
			VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

			bool allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

			void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

			void resetPool();

		private:
			VulkanDevice& rDevice;
			VkDescriptorPool mDescriptorPool;

			friend class VulkanDescriptorWriter;
		};

		class VulkanDescriptorWriter {
		public:
			VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

			VulkanDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
			VulkanDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

			bool build(VkDescriptorSet& set);
			void overwrite(VkDescriptorSet& set);
		private:
			VulkanDescriptorSetLayout& rSetLayout;
			VulkanDescriptorPool& rPool;
			std::vector<VkWriteDescriptorSet> mWrites;
		};
	}
}