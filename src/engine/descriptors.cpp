#include "descriptors.h"
#include "./util/debug.h"

namespace engine {
	/*************************************
		* LightningDescriptorSetLayout.Builder
		*************************************/

	VulkanDescriptorSetLayout::Builder& VulkanDescriptorSetLayout::Builder::addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count) {
		if (mBindings.count(binding) != 0) {
			util::displayError("could not add binding because binding already in use");
		}

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		mBindings[binding] = layoutBinding;

		return *this;
	}

	std::unique_ptr<VulkanDescriptorSetLayout> VulkanDescriptorSetLayout::Builder::build() const {
		return std::make_unique<VulkanDescriptorSetLayout>(rDevice, mBindings);
	}

	/*****************************
	* LightningDescriptorSetLayout
	*****************************/

	VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings) : rDevice{ device }, mBindings{ bindings } {
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

		for (auto& kv : mBindings) {
			setLayoutBindings.push_back(kv.second);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(rDevice.getDevice(), &descriptorSetLayoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
			util::displayError("Failed to create descriptor set layout");
		}
	}

	VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
		vkDestroyDescriptorSetLayout(rDevice.getDevice(), mDescriptorSetLayout, nullptr);
	}

	/********************************
	* LightningDescriptorPool.Builder
	********************************/

	VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count) {
		mPoolSizes.push_back({ descriptorType, count });
		return *this;
	}

	VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
		mPoolFlags = flags;
		return *this;
	}

	VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::setMaxSets(uint32_t count) {
		mMaxSets = count;
		return *this;
	}

	std::unique_ptr<VulkanDescriptorPool> VulkanDescriptorPool::Builder::build() const {
		return std::make_unique<VulkanDescriptorPool>(rDevice, mMaxSets, mPoolFlags, mPoolSizes);
	}

	/************************
	* LightningDescriptorPool
	************************/

	VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes) : rDevice{ device } {
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(rDevice.getDevice(), &descriptorPoolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS) {
			util::displayError("Failed to create descriptor pool");
		}
	}

	VulkanDescriptorPool::~VulkanDescriptorPool() {
		vkDestroyDescriptorPool(rDevice.getDevice(), mDescriptorPool, nullptr);
	}

	bool VulkanDescriptorPool::allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// TODO create a DescriptorPoolManager class to handle cases where pools fill up

		if (vkAllocateDescriptorSets(rDevice.getDevice(), &allocInfo, &descriptor) != VK_SUCCESS) {
			return false;
		}
		else {
			return true;
		}
	}

	void VulkanDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
		vkFreeDescriptorSets(rDevice.getDevice(), mDescriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());
	}

	void VulkanDescriptorPool::resetPool() {
		vkResetDescriptorPool(rDevice.getDevice(), mDescriptorPool, 0);
	}

	/**************************
	* LightningDescriptorWriter
	**************************/

	VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool) : rSetLayout{ setLayout }, rPool{ pool } {}

	VulkanDescriptorWriter& VulkanDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
		if (rSetLayout.mBindings.count(binding) != 1) {
			util::displayError("could not write to buffer because layout does not contain specified binding!");
		}

		auto& bindingDescription = rSetLayout.mBindings[binding];

		if (bindingDescription.descriptorCount != 1) {
			util::displayError("attempted to bind single descriptor info, but binding expects multiple");
		}

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		mWrites.push_back(write);
		return *this;
	}

	VulkanDescriptorWriter& VulkanDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) {
		if (rSetLayout.mBindings.count(binding) != 1) {
			util::displayError("could not write to image because layout does not contain specified binding");
		}

		auto& bindingDescription = rSetLayout.mBindings[binding];

		if (bindingDescription.descriptorCount != 1) {
			util::displayError("attempted to bind single descriptor info, but binding expects multiple");
		}

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;

		mWrites.push_back(write);
		return *this;
	}

	bool VulkanDescriptorWriter::build(VkDescriptorSet& set) {
		bool success = rPool.allocateDescriptorSet(rSetLayout.getDescriptorSetLayout(), set);

		if (!success) {
			return false;
		}

		overwrite(set);
		return true;
	}

	void VulkanDescriptorWriter::overwrite(VkDescriptorSet& set) {
		for (auto& write : mWrites) {
			write.dstSet = set;
		}

		vkUpdateDescriptorSets(rPool.rDevice.getDevice(), mWrites.size(), mWrites.data(), 0, nullptr);
	}
}