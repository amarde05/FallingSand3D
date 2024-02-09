#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine {
	class PipelineBuilder {
	public:
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineLayout pipelineLayout;

		VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);

		static VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
		static VkPipelineVertexInputStateCreateInfo getVertexInputStateCreateInfo();
		static VkPipelineInputAssemblyStateCreateInfo getInputAssemblyCreateInfo(VkPrimitiveTopology topology);
		static VkPipelineRasterizationStateCreateInfo getRasterizationStateCreateInfo(VkPolygonMode polygonMode);
		static VkPipelineMultisampleStateCreateInfo getMultisamplingStateCreateInfo();
		static VkPipelineColorBlendAttachmentState getColorBlendAttachmentState();
		static VkPipelineLayoutCreateInfo getPipelineLayoutCreateInfo();
	};
}