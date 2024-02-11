#include "pipelines.h"
#include "util/debug.h"
#include "mesh.h"

namespace engine {
	namespace rendering {
		VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass) {
			// Make viewport state from stored viewport and scissor
			// Does not yet support multiple viewports or scissors
			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.pNext = nullptr;

			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			// Setup dummy color blending (does not blend transparent objects yet)
			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.pNext = nullptr;

			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;

			// Build the pipeline
			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pNext = nullptr;

			pipelineInfo.stageCount = shaderStages.size();
			pipelineInfo.pStages = shaderStages.data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = pass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			VkPipeline newPipeline;
			if (vkCreateGraphicsPipelines(
				device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
				util::displayMessage("Failed to create pipeline", DISPLAY_TYPE_ERR);
				return VK_NULL_HANDLE;
			}
			else {
				return newPipeline;
			}
		}

		VkPipelineShaderStageCreateInfo PipelineBuilder::getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;

			info.stage = stage;
			info.module = shaderModule;

			// Hardcodes the entry point of the shader to the function 'main'
			info.pName = "main";

			return info;
		}

		// Doesn't work for some reason
		VkPipelineVertexInputStateCreateInfo PipelineBuilder::getVertexInputStateCreateInfo() {
			VertexInputDescription vertexInputDescription = Vertex::getVertexDescription();


			VkPipelineVertexInputStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.vertexAttributeDescriptionCount = vertexInputDescription.attributes.size();
			info.pVertexAttributeDescriptions = vertexInputDescription.attributes.data();

			info.vertexBindingDescriptionCount = vertexInputDescription.bindings.size();
			info.pVertexBindingDescriptions = vertexInputDescription.bindings.data();

			return info;
		}

		VkPipelineInputAssemblyStateCreateInfo PipelineBuilder::getInputAssemblyCreateInfo(VkPrimitiveTopology topology) {
			VkPipelineInputAssemblyStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.topology = topology;

			info.primitiveRestartEnable = VK_FALSE;

			return info;
		}

		VkPipelineRasterizationStateCreateInfo PipelineBuilder::getRasterizationStateCreateInfo(VkPolygonMode polygonMode) {
			VkPipelineRasterizationStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthClampEnable = VK_FALSE;
			info.rasterizerDiscardEnable = VK_FALSE;

			info.polygonMode = polygonMode;
			info.lineWidth = 1.0f;
			info.cullMode = VK_CULL_MODE_NONE;
			info.frontFace = VK_FRONT_FACE_CLOCKWISE;
			info.depthBiasEnable = VK_FALSE;
			info.depthBiasConstantFactor = 0.0f;
			info.depthBiasClamp = 0.0f;
			info.depthBiasSlopeFactor = 0.0f;

			return info;
		}

		VkPipelineMultisampleStateCreateInfo PipelineBuilder::getMultisamplingStateCreateInfo() {
			VkPipelineMultisampleStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.sampleShadingEnable = VK_FALSE;
			// Change to enable multisampling
			info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			info.minSampleShading = 1.0f;
			info.pSampleMask = nullptr;
			info.alphaToCoverageEnable = VK_FALSE;
			info.alphaToOneEnable = VK_FALSE;

			return info;
		}

		VkPipelineColorBlendAttachmentState PipelineBuilder::getColorBlendAttachmentState() {
			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			return colorBlendAttachment;
		}

		VkPipelineDepthStencilStateCreateInfo PipelineBuilder::getDepthStencilState(bool depthTest, bool depthWrite, VkCompareOp compareOp) {
			VkPipelineDepthStencilStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
			info.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
			info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
			info.depthBoundsTestEnable = VK_FALSE;
			info.minDepthBounds = 0.0f;
			info.maxDepthBounds = 1.0f;
			info.stencilTestEnable = VK_FALSE;

			return info;
		}

		VkPipelineLayoutCreateInfo PipelineBuilder::getPipelineLayoutCreateInfo() {
			VkPipelineLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;

			info.flags = 0;
			info.setLayoutCount = 0;
			info.pSetLayouts = nullptr;
			info.pushConstantRangeCount = 0;
			info.pPushConstantRanges = nullptr;

			return info;
		}
	}
}