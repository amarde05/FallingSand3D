#include "materials.h"
#include "pipelines.h"
#include "mesh.h"
#include "../window.h"
#include "../../util/debug.h"

#include <glm/gtx/transform.hpp>

#include <fstream>

#define MAX_OBJECTS 10000

namespace engine {
	namespace rendering {
		std::unique_ptr<VulkanDescriptorSetLayout> globalDescriptorSetLayout;
		VkDescriptorSet globalDescriptorSet;
		memory::AllocatedBuffer globalBuffer;

		// Pass descriptor set may be used later?
		// static std::unique_ptr<VulkanDescriptorSetLayout> sPassDescriptorSetLayout;
		// static VkDescriptorSet sPassDescriptorSet;

		std::unique_ptr<VulkanDescriptorSetLayout> objectDescriptorSetLayout;
		std::vector<VkDescriptorSet> objectDescriptorSets;
		memory::AllocatedBuffer objectBuffer;

		std::unique_ptr<VulkanDescriptorPool> globalDescriptorPool;

		VulkanDevice* pDevice;

		std::unordered_map<std::string, Material*> materials;

		int frameOverlap;


		// Base Material class functions
		Material::Material(std::string name, std::string fragName, std::string VertName) : mName{ name }, mFragFileName{ fragName }, mVertFileName{ VertName } {
			materials[mName] = this;
		}

		void Material::initPipeline(VkRenderPass renderPass) {
			// Create shader modules
			VkShaderModule fragShader;
			if (!loadShaderModule("../../shaders/basic_mat.frag.spv", &fragShader)) {
				util::displayMessage("Failed to build the fragment shader module", DISPLAY_TYPE_ERR);
			}
			else {
				util::displayMessage("Fragment shader successfully loaded", DISPLAY_TYPE_INFO);
			}

			VkShaderModule vertexShader;
			if (!loadShaderModule("../../shaders/basic_mat.vert.spv", &vertexShader)) {
				util::displayMessage("Failed to build the vertex shader module", DISPLAY_TYPE_ERR);
			}
			else {
				util::displayMessage("Vertex shader successfully loaded", DISPLAY_TYPE_INFO);
			}


			// Create pipeline layout
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineBuilder::getPipelineLayoutCreateInfo();

			// Setup push constants
			/*VkPushConstantRange pushConstant;
			pushConstant.offset = 0;
			pushConstant.size = sizeof(MeshPushConstants);
			pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstant;*/

			VkDescriptorSetLayout setLayouts[]{ globalDescriptorSetLayout->getDescriptorSetLayout(),
												mMaterialDescriptorSetLayout->getDescriptorSetLayout(),
												objectDescriptorSetLayout->getDescriptorSetLayout() };

			pipelineLayoutInfo.setLayoutCount = 3;
			pipelineLayoutInfo.pSetLayouts = setLayouts;

			if (vkCreatePipelineLayout(pDevice->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS) {
				util::displayError("Failed to create pipeline layout");
			}


			// Create pipeline object
			PipelineBuilder pipelineBuilder;

			pipelineBuilder.shaderStages.push_back(PipelineBuilder::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
			pipelineBuilder.shaderStages.push_back(PipelineBuilder::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader));

			VertexInputDescription vertexInputDescription = Vertex::getVertexDescription();


			VkPipelineVertexInputStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.vertexAttributeDescriptionCount = vertexInputDescription.attributes.size();
			info.pVertexAttributeDescriptions = vertexInputDescription.attributes.data();

			info.vertexBindingDescriptionCount = vertexInputDescription.bindings.size();
			info.pVertexBindingDescriptions = vertexInputDescription.bindings.data();

			pipelineBuilder.vertexInputInfo = info;

			pipelineBuilder.inputAssembly = PipelineBuilder::getInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			// Build viewport and scissor
			VkExtent2D windowExtent = Window::getMainWindow().getExtent();

			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)windowExtent.width;
			pipelineBuilder.viewport.height = (float)windowExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = windowExtent;

			pipelineBuilder.rasterizer = PipelineBuilder::getRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

			pipelineBuilder.multisampling = PipelineBuilder::getMultisamplingStateCreateInfo();

			pipelineBuilder.colorBlendAttachment = PipelineBuilder::getColorBlendAttachmentState();

			pipelineBuilder.depthStencil = PipelineBuilder::getDepthStencilState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			pipelineBuilder.pipelineLayout = mPipelineLayout;

			mPipeline = pipelineBuilder.buildPipeline(pDevice->getDevice(), renderPass);

			vkDestroyShaderModule(pDevice->getDevice(), fragShader, nullptr);
			vkDestroyShaderModule(pDevice->getDevice(), vertexShader, nullptr);
		}

		void Material::cleanup() {
			mMaterialDescriptorSetLayout = nullptr;

			vkDestroyPipeline(pDevice->getDevice(), mPipeline, nullptr);
			vkDestroyPipelineLayout(pDevice->getDevice(), mPipelineLayout, nullptr);
		}

		void Material::bind(VkCommandBuffer cmd, int frameIndex) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

			// Bind descriptor sets
			uint32_t globalDataOffset = padUniformBufferSize(sizeof(GlobalData)) * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &globalDescriptorSet, 1, &globalDataOffset);

			bindMaterialDescriptors(cmd, frameIndex);

			// Object Data descriptor
			uint32_t objectDataOffset = sizeof(ObjectData) * MAX_OBJECTS * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 2, 1, &objectDescriptorSets[frameIndex], 1, &objectDataOffset);
		}

		bool Material::loadShaderModule(std::string filePath, VkShaderModule* outShaderModule) const {
			// Open the file in binary with the cursor at the end
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				util::displayMessage("File not found: " + filePath, DISPLAY_TYPE_ERR);
				return false;
			}

			// Find the size of the file by looking up the location of the cursor
			// Since the cursor starts at the end, it gives the size directly in bytes
			size_t fileSize = (size_t)file.tellg();

			// Spirv expects the buffer to be on uint32
			std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

			// Put file cursor at beginning
			file.seekg(0);

			// Load entire file into the buffer
			file.read((char*)buffer.data(), fileSize);

			file.close();


			// Now create shader module
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;

			createInfo.codeSize = buffer.size() * sizeof(uint32_t);
			createInfo.pCode = buffer.data();

			VkShaderModule shaderModule;

			if (vkCreateShaderModule(pDevice->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				util::displayMessage("Failed to create shader module", DISPLAY_TYPE_ERR);
				return false;
			}

			*outShaderModule = shaderModule;

			return true;
		}


		// BasicMaterial class
		BasicMaterial::BasicMaterial(std::string name, BasicData data) : Material(name, "basic_mat", "basic_mat"), mData{data} {

		}

		
		void BasicMaterial::initDescriptors() {
			VulkanDescriptorSetLayout::Builder setBuilder(*pDevice);

			setBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

			mMaterialDescriptorSetLayout = setBuilder.build();

			mDataBuffer = memory::createBuffer(sizeof(BasicData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			VulkanDescriptorWriter setWriter(*mMaterialDescriptorSetLayout.get(), *globalDescriptorPool.get());

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = mDataBuffer.buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(BasicData);

			setWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);

			setWriter.build(mMaterialDescriptorSet);

			memory::getAllocationDeletionQueue().pushFunction([=]() {
				vmaDestroyBuffer(memory::getAllocator(), mDataBuffer.buffer, mDataBuffer.allocation);
			});
		}

		void BasicMaterial::writeData() {
			void* data;
			vmaMapMemory(memory::getAllocator(), mDataBuffer.allocation, &data);

			BasicData* basicData = (BasicData*)data;

			basicData->color = mData.color;

			vmaUnmapMemory(memory::getAllocator(), mDataBuffer.allocation);
		}

		void BasicMaterial::bindMaterialDescriptors(VkCommandBuffer cmd, int frameIndex) {
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1, 1, &mMaterialDescriptorSet, 0, nullptr);
		}


		// Static functions
		size_t Material::padUniformBufferSize(size_t originalSize) {
			size_t minUboAlignment = pDevice->getDeviceProperties().gpuProperties.limits.minUniformBufferOffsetAlignment;
			size_t alignedSize = originalSize;

			if (minUboAlignment > 0) {
				alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			return alignedSize;
		}

		void Material::initializeMaterials(VulkanDevice* device, VkRenderPass renderPass, int numFrames) {
			pDevice = device;
			frameOverlap = numFrames;

			// Build descriptor pool
			VulkanDescriptorPool::Builder poolBuilder(*pDevice);

			poolBuilder.setPoolFlags(0);

			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10);
			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10);
			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10);
			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10);

			globalDescriptorPool = poolBuilder.build();



			// Build global set layout
			VulkanDescriptorSetLayout::Builder globalSetBuilder(*pDevice);

			globalSetBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

			globalDescriptorSetLayout = globalSetBuilder.build();

			// Create global buffer
			const size_t globalBufferSize = frameOverlap * padUniformBufferSize(sizeof(GlobalData));
			globalBuffer = memory::createBuffer(globalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);			

			// Write to global set
			VulkanDescriptorWriter globalWriter(*globalDescriptorSetLayout.get(), *globalDescriptorPool.get());
			
			VkDescriptorBufferInfo globalBufferInfo{};
			globalBufferInfo.buffer = globalBuffer.buffer;
			globalBufferInfo.offset = 0;
			globalBufferInfo.range = sizeof(GlobalData);

			globalWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &globalBufferInfo);

			globalWriter.build(globalDescriptorSet);



			// Build object descriptor set layout
			VulkanDescriptorSetLayout::Builder objectSetBuilder(*pDevice);

			objectSetBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1);

			objectDescriptorSetLayout = objectSetBuilder.build();
			

			// Create object buffer
			objectBuffer = memory::createBuffer(frameOverlap * sizeof(ObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			// Write to object sets
			VulkanDescriptorWriter objectWriter(*objectDescriptorSetLayout.get(), *globalDescriptorPool.get());

			VkDescriptorBufferInfo objectBufferInfo{};
			objectBufferInfo.buffer = objectBuffer.buffer;
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(ObjectData) * MAX_OBJECTS;

			objectWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, &objectBufferInfo);

			objectDescriptorSets.resize(frameOverlap);

			for (int i = 0; i < frameOverlap; i++) {
				objectWriter.build(objectDescriptorSets[i]);
			}

			
			// Call the initDescriptors function in each registered material
			for (auto& entry : materials) {
				Material* mat = entry.second;

				mat->initDescriptors();
				mat->initPipeline(renderPass);
			}


			// Add buffers to deletion queue
			memory::getAllocationDeletionQueue().pushFunction([=]() {
				vmaDestroyBuffer(memory::getAllocator(), globalBuffer.buffer, globalBuffer.allocation);
				vmaDestroyBuffer(memory::getAllocator(), objectBuffer.buffer, objectBuffer.allocation);
			});
		}

		std::unique_ptr<BasicMaterial> mat;

		void Material::createMaterials() {
			BasicData data{};
			data.color = glm::vec4{ 1.0f, 0.5f, 0.5f, 1.0f };

			mat = std::make_unique<BasicMaterial>("default", data);
		}

		void Material::writeGlobalAndObjectData(int frameIndex, int objectCount, RenderObject* first, glm::vec3 camPos) {
			glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);
			glm::mat4 proj = glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.01f, 200.0f);
			proj[1][1] *= -1;

			glm::mat4 viewproj = proj * view;

			GlobalData globalDataStruct;
			globalDataStruct.cameraPos = { camPos.x, camPos.y, camPos.z, 0.0f };
			globalDataStruct.proj = proj;
			globalDataStruct.view = view;
			globalDataStruct.viewproj = viewproj;

			// Copy data to the buffer
			char* globalData;
			vmaMapMemory(memory::getAllocator(), globalBuffer.allocation, (void**)&globalData);

			globalData += padUniformBufferSize(sizeof(GlobalData)) * frameIndex;

			memcpy(globalData, &globalDataStruct, sizeof(GlobalData));

			vmaUnmapMemory(memory::getAllocator(), globalBuffer.allocation);


			// Copy object data to object buffer
			void* objectData;
			vmaMapMemory(memory::getAllocator(), objectBuffer.allocation, &objectData);

			ObjectData* objectSSBO = (ObjectData*)objectData;

			/*glm::mat4 rotation = glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(1, 0, 0))*
				glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(0, 1, 0))*
				glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(0, 0, 1));*/

			for (int i = 0; i < objectCount; i++) {
				RenderObject& object = first[i];
				objectSSBO[i + MAX_OBJECTS * frameIndex].transformMatrix = object.transformMatrix;
			}

			vmaUnmapMemory(memory::getAllocator(), objectBuffer.allocation);
		}

		void Material::cleanupMaterials() {
			globalDescriptorSetLayout = nullptr;
			//sPassDescriptorSetLayout = nullptr;
			objectDescriptorSetLayout = nullptr;
			
			for (auto& entry : materials) {
				Material* mat = entry.second;

				mat->cleanup();
			}
			
			globalDescriptorPool = nullptr;
		}

		Material* Material::getMaterial(const std::string& name) {
			auto it = materials.find(name);

			if (it == materials.end()) {
				return nullptr;
			}
			else {
				return it->second;
			}
		}
	}
}