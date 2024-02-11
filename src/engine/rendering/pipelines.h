#pragma once
#include <vulkan/vulkan.h>

#include <vector>

namespace engine {
	namespace rendering {
		class PipelineBuilder {
		public:
			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			VkPipelineVertexInputStateCreateInfo vertexInputInfo;
			VkPipelineInputAssemblyStateCreateInfo inputAssembly;
			VkViewport viewport;
			VkRect2D scissor;
			VkPipelineRasterizationStateCreateInfo rasterizer;
			VkPipelineColorBlendAttachmentState colorBlendAttachment;
			VkPipelineDepthStencilStateCreateInfo depthStencil;
			VkPipelineMultisampleStateCreateInfo multisampling;
			VkPipelineLayout pipelineLayout;

			VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);

			static VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
			static VkPipelineVertexInputStateCreateInfo getVertexInputStateCreateInfo();
			static VkPipelineInputAssemblyStateCreateInfo getInputAssemblyCreateInfo(VkPrimitiveTopology topology);
			static VkPipelineRasterizationStateCreateInfo getRasterizationStateCreateInfo(VkPolygonMode polygonMode);
			static VkPipelineMultisampleStateCreateInfo getMultisamplingStateCreateInfo();
			static VkPipelineColorBlendAttachmentState getColorBlendAttachmentState();
			static VkPipelineDepthStencilStateCreateInfo getDepthStencilState(bool depthTest, bool depthWrite, VkCompareOp compareOp);
			static VkPipelineLayoutCreateInfo getPipelineLayoutCreateInfo();
		};
	}
}