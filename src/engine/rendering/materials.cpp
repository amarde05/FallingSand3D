#include "materials.h"
#include "pipelines.h"
#include "textures.h"
#include "mesh.h"
#include "tools/initializers.h"
#include "../window.h"
#include "../../util/debug.h"

#include <glm/gtx/transform.hpp>

#include <fstream>

#define MAX_OBJECTS 10000

namespace engine {
	namespace rendering {
		// MaterialLayout struct
		std::unique_ptr<VulkanDescriptorSetLayout> MaterialLayout::sGlobalSetLayout;
		std::unique_ptr<VulkanDescriptorSetLayout> MaterialLayout::sObjectSetLayout;

		MaterialLayout::MaterialLayout(std::string vertFileName, std::string fragFileName) : vertFileName{ vertFileName }, fragFileName{ fragFileName } {
			pipeline = VK_NULL_HANDLE;
			pipelineLayout = VK_NULL_HANDLE;
		}

		MaterialLayout& MaterialLayout::addDataBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, size_t bufferSize, size_t bufferRange, uint32_t count) {
			VkDescriptorSetLayoutBinding bind{};
			bind.binding = binding;
			bind.descriptorType = descriptorType;
			bind.stageFlags = stageFlags;
			bind.descriptorCount = count;
			
			MaterialLayoutBinding layoutBinding{};
			layoutBinding.binding = bind;
			layoutBinding.bindingType = BINDING_TYPE_DATA;
			layoutBinding.bufferSize = bufferSize;
			layoutBinding.bufferRange = bufferRange;

			bindings.push_back(layoutBinding);

			return *this;
		}

		MaterialLayout& MaterialLayout::addImageBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, VkImageLayout imageLayout, uint32_t count) {
			VkDescriptorSetLayoutBinding bind{};
			bind.binding = binding;
			bind.descriptorType = descriptorType;
			bind.stageFlags = stageFlags;
			bind.descriptorCount = count;

			MaterialLayoutBinding layoutBinding{};
			layoutBinding.binding = bind;
			layoutBinding.bindingType = BINDING_TYPE_IMAGE;
			layoutBinding.imageLayout = imageLayout;

			bindings.push_back(layoutBinding);

			return *this;
		}

		void MaterialLayout::createDescriptorLayout(VulkanDevice* pDevice) {
			VulkanDescriptorSetLayout::Builder setBuilder(*pDevice);

			for (auto& i : bindings) {
				setBuilder.addBinding(i.binding.binding, i.binding.descriptorType, i.binding.stageFlags, i.binding.descriptorCount);
			}

			setLayout = setBuilder.build();
		}

		void MaterialLayout::createPipeline(VulkanDevice* pDevice, VkRenderPass renderPass) {
			// Create shader modules
			VkShaderModule fragShader;
			if (!loadShaderModule(pDevice, "../../shaders/" + fragFileName + ".frag.spv", &fragShader)) {
				util::displayMessage("Failed to build the fragment shader module", DISPLAY_TYPE_ERR);
			}
			else {
				util::displayMessage("Fragment shader successfully loaded", DISPLAY_TYPE_INFO);
			}

			VkShaderModule vertexShader;
			if (!loadShaderModule(pDevice, "../../shaders/" + vertFileName + ".vert.spv", &vertexShader)) {
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

			VkDescriptorSetLayout setLayouts[]{ sGlobalSetLayout->getDescriptorSetLayout(),
												sObjectSetLayout->getDescriptorSetLayout(),
												setLayout->getDescriptorSetLayout()
			};

			pipelineLayoutInfo.setLayoutCount = 3;
			pipelineLayoutInfo.pSetLayouts = setLayouts;

			if (vkCreatePipelineLayout(pDevice->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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

			pipelineBuilder.pipelineLayout = pipelineLayout;

			pipeline = pipelineBuilder.buildPipeline(pDevice->getDevice(), renderPass);

			vkDestroyShaderModule(pDevice->getDevice(), fragShader, nullptr);
			vkDestroyShaderModule(pDevice->getDevice(), vertexShader, nullptr);
		}

		MaterialLayout& MaterialLayout::finalize(VulkanDevice* pDevice, VkRenderPass renderPass) {
			createDescriptorLayout(pDevice);
			createPipeline(pDevice, renderPass);

			return *this;
		}

		void MaterialLayout::cleanup(VulkanDevice* pDevice) {
			setLayout = nullptr;

			vkDestroyPipeline(pDevice->getDevice(), pipeline, nullptr);
			vkDestroyPipelineLayout(pDevice->getDevice(), pipelineLayout, nullptr);
		}


		void MaterialLayout::createStaticLayouts(VulkanDevice* pDevice) {
			// Create global set layout
			VulkanDescriptorSetLayout::Builder globalSetBuilder(*pDevice);

			globalSetBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

			sGlobalSetLayout = globalSetBuilder.build();

			// Create object set layout
			VulkanDescriptorSetLayout::Builder objectSetBuilder(*pDevice);

			objectSetBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1);

			sObjectSetLayout = objectSetBuilder.build();
		}

		void MaterialLayout::cleanupStaticLayouts() {
			sGlobalSetLayout = nullptr;
			sObjectSetLayout = nullptr;
		}

		bool MaterialLayout::loadShaderModule(VulkanDevice* pDevice, std::string filePath, VkShaderModule* outShaderModule)  {
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


		// Material class
		VkDescriptorSet Material::sGlobalDescriptorSet;
		memory::AllocatedBuffer Material::sGlobalBuffer;

		std::vector<VkDescriptorSet> Material::sObjectDescriptorSets;
		memory::AllocatedBuffer Material::sObjectBuffer;

		std::unique_ptr<VulkanDescriptorPool> Material::sGlobalDescriptorPool;

		VulkanDevice* Material::pDevice;

		std::unordered_map<std::string, std::unique_ptr<Material>> Material::sMaterials;

		std::unordered_map<std::string, std::unique_ptr<MaterialLayout>> Material::sMaterialLayouts;

		VkSampler Material::sGlobalSampler;

		int Material::sFrameOverlap;


		Material* Material::create(std::string name, MaterialLayout* pLayout) {
			sMaterials[name] = std::make_unique<Material>(name, pLayout);

			sMaterials[name]->createMemoryObjects();

			return sMaterials[name].get();
		}

		MaterialLayout* Material::createMaterialLayout(std::string name, std::string vertFileName, std::string fragFileName) {
			sMaterialLayouts[name] = std::make_unique<MaterialLayout>(vertFileName, fragFileName);

			return sMaterialLayouts[name].get();
		}

		Material::Material(std::string name, MaterialLayout* layout) : mName{ name }, pMaterialLayout{ layout } {
		}

		void Material::cleanup() {
			
		}

		void Material::bindGlobalSet(VkCommandBuffer cmd, int frameIndex) {
			// Bind descriptor sets
			uint32_t globalDataOffset = padUniformBufferSize(sizeof(GlobalData)) * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pMaterialLayout->pipelineLayout, 0, 1, &sGlobalDescriptorSet, 1, &globalDataOffset);
		}

		void Material::bindObjectSet(VkCommandBuffer cmd, int frameIndex) {
			// Object Data descriptor
			uint32_t objectDataOffset = sizeof(ObjectData) * MAX_OBJECTS * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pMaterialLayout->pipelineLayout, 1, 1, &sObjectDescriptorSets[frameIndex], 1, &objectDataOffset);
		}

		void Material::bind(VkCommandBuffer cmd, int frameIndex) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pMaterialLayout->pipeline);

			if (pMaterialLayout->bindings.size() > 0)
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pMaterialLayout->pipelineLayout, 2, 1, &mMaterialDescriptorSet, 0, nullptr);
		}

		void Material::createMemoryObjects() {
			if (pMaterialLayout->bindings.size() == 0)
				return;

			mMemoryObjects.resize(pMaterialLayout->bindings.size());

			for (auto& i : pMaterialLayout->bindings) {
				MaterialMemoryObject newMemObj{};

				newMemObj.bindingType = i.bindingType;

				switch (i.bindingType) {
				case BINDING_TYPE_DATA: {
					VkBufferUsageFlags usageFlags;

					switch (i.binding.descriptorType) {
					default:
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
						usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
						usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
						break;
					}

					newMemObj.buffer = memory::createBuffer(i.bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

					VkDescriptorBufferInfo bInfo{};
					bInfo.buffer = newMemObj.buffer.buffer;
					bInfo.offset = 0;
					bInfo.range = i.bufferRange;

					newMemObj.bufferInfo = bInfo;

					memory::getAllocationDeletionQueue().pushFunction([=]() {
						vmaDestroyBuffer(memory::getAllocator(), newMemObj.buffer.buffer, newMemObj.buffer.allocation);
						});

				} break;
				case BINDING_TYPE_IMAGE: {

					VkDescriptorImageInfo dimgInfo{};
					dimgInfo.sampler = sGlobalSampler;
					dimgInfo.imageLayout = i.imageLayout;

					newMemObj.imageInfo = dimgInfo;
				} break;
				}


				mMemoryObjects[i.binding.binding] = newMemObj;
			}
		}

		void Material::initDescriptors() {
			if (pMaterialLayout->bindings.size() == 0)
				return;

			VulkanDescriptorWriter setWriter(*pMaterialLayout->setLayout.get(), *sGlobalDescriptorPool.get());

			for (auto& i : pMaterialLayout->bindings) {
				switch (i.bindingType) {
				case BINDING_TYPE_DATA:
					setWriter.writeBuffer(i.binding.binding, i.binding.descriptorType, &mMemoryObjects[i.binding.binding].bufferInfo);

					break;
				case BINDING_TYPE_IMAGE:
					setWriter.writeImage(i.binding.binding, i.binding.descriptorType, &mMemoryObjects[i.binding.binding].imageInfo);
					break;
				}
			}

			setWriter.build(mMaterialDescriptorSet);
		}

		Material& Material::writeBuffer(uint32_t binding, void* data) {
			MaterialMemoryObject memObj = mMemoryObjects[binding];

			if (memObj.bindingType != BINDING_TYPE_DATA) {
				util::displayError("The object at binding " + std::to_string(binding) + " must be a buffer!");
			}

			void* d;
			
			vmaMapMemory(memory::getAllocator(), memObj.buffer.allocation, &d);

			memcpy(d, data, memObj.bufferInfo.range);

			vmaUnmapMemory(memory::getAllocator(), memObj.buffer.allocation);

			return *this;
		}

		Material& Material::writeImage(uint32_t binding, std::string textureName) {
			Texture* tex = getTexture(textureName);

			mMemoryObjects[binding].imageInfo.imageView = tex->imageView;

			return *this;
		}

		Material& Material::finalize() {
			initDescriptors();

			return *this;
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
			sFrameOverlap = numFrames;

			// Initialize sampler
			VkSamplerCreateInfo samplerInfo = tools::createSamplerInfo(VK_FILTER_NEAREST);

			vkCreateSampler(pDevice->getDevice(), &samplerInfo, nullptr, &sGlobalSampler);

			// Build descriptor pool
			VulkanDescriptorPool::Builder poolBuilder(*pDevice);

			poolBuilder.setPoolFlags(0);

			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10);
			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10);
			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10);
			poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10);

			sGlobalDescriptorPool = poolBuilder.build();

			// Build set layouts
			MaterialLayout::createStaticLayouts(pDevice);

			// Create global buffer
			const size_t globalBufferSize = sFrameOverlap * padUniformBufferSize(sizeof(GlobalData));
			sGlobalBuffer = memory::createBuffer(globalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);			

			// Write to global set
			VulkanDescriptorWriter globalWriter(*MaterialLayout::sGlobalSetLayout.get(), *sGlobalDescriptorPool.get());
			
			VkDescriptorBufferInfo globalBufferInfo{};
			globalBufferInfo.buffer = sGlobalBuffer.buffer;
			globalBufferInfo.offset = 0;
			globalBufferInfo.range = sizeof(GlobalData);

			globalWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &globalBufferInfo);

			globalWriter.build(sGlobalDescriptorSet);

			// Create object buffer
			sObjectBuffer = memory::createBuffer(sFrameOverlap * sizeof(ObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			// Write to object sets
			VulkanDescriptorWriter objectWriter(*MaterialLayout::sObjectSetLayout.get(), *sGlobalDescriptorPool.get());

			VkDescriptorBufferInfo objectBufferInfo{};
			objectBufferInfo.buffer = sObjectBuffer.buffer;
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(ObjectData) * MAX_OBJECTS;

			objectWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, &objectBufferInfo);

			sObjectDescriptorSets.resize(sFrameOverlap);

			for (int i = 0; i < sFrameOverlap; i++) {
				objectWriter.build(sObjectDescriptorSets[i]);
			}

			// Create materials
			BasicData data1{};
			data1.color = glm::vec4{ 1.0f, 0.5f, 0.5f, 1.0f };

			BasicData data2{};
			data2.color = glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f };

			Material::createMaterialLayout("basic_layout", "basic_mat", "basic_mat")
				->addDataBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, padUniformBufferSize(sizeof(BasicData)), sizeof(BasicData))
				.finalize(pDevice, renderPass);

			Material::create("red", Material::getMaterialLayout("basic_layout"))->writeBuffer(0, &data1).finalize();
			Material::create("green", Material::getMaterialLayout("basic_layout"))->writeBuffer(0, &data2).finalize();

			Material::createMaterialLayout("textured_layout", "textured_mat", "textured_mat")
				->addDataBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, padUniformBufferSize(sizeof(BasicData)), sizeof(BasicData))
				.addImageBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.finalize(pDevice, renderPass);

			Material::create("textured_red", Material::getMaterialLayout("textured_layout"))
				->writeBuffer(0, &data1)
				.writeImage(1, "rubiks")
				.finalize();

			Material::createMaterialLayout("raymarch_layout", "raymarch_sphere", "raymarch_sphere")->finalize(pDevice, renderPass);

			Material::create("raymarch_sphere", Material::getMaterialLayout("raymarch_layout"))->finalize();

			Material::createMaterialLayout("fs_raymarch_layout", "fs_raymarch", "fs_raymarch")->finalize(pDevice, renderPass);
			Material::create("fs_raymarch_mat", Material::getMaterialLayout("fs_raymarch_layout"))->finalize();


			// Add buffers to deletion queue
			memory::getAllocationDeletionQueue().pushFunction([=]() {
				vmaDestroyBuffer(memory::getAllocator(), sGlobalBuffer.buffer, sGlobalBuffer.allocation);
				vmaDestroyBuffer(memory::getAllocator(), sObjectBuffer.buffer, sObjectBuffer.allocation);
			});
		}

		void Material::writeGlobalAndObjectData(int frameIndex, int objectCount, RenderObject* first, glm::vec3 camPos, glm::vec3 camRot) {
			glm::mat4 view = glm::rotate(glm::mat4{ 1.0f }, glm::radians(0.0f), glm::vec3(1, 0, 0)) * glm::rotate(glm::mat4{ 1.0f }, glm::radians(camRot.y), glm::vec3(0, 1, 0)) * glm::translate(glm::mat4(1.0f), camPos);
			glm::mat4 proj = glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.01f, 200.0f);
			proj[1][1] *= -1;

			glm::mat4 viewproj = proj * view;

			GlobalData globalDataStruct;
			globalDataStruct.cameraPos = { camPos.x, camPos.y, camPos.z, 1.0f };
			globalDataStruct.aspect = { 1700.0f / 900.0f, 850, 450, 0 };
			globalDataStruct.camDir = { 0, 0, 1, 1 };
			globalDataStruct.proj = proj;
			globalDataStruct.view = view;
			globalDataStruct.viewproj = viewproj;
			globalDataStruct.invViewProj = glm::inverse(viewproj);

			// Copy data to the buffer
			char* globalData;
			vmaMapMemory(memory::getAllocator(), sGlobalBuffer.allocation, (void**)&globalData);

			globalData += padUniformBufferSize(sizeof(GlobalData)) * frameIndex;

			memcpy(globalData, &globalDataStruct, sizeof(GlobalData));

			vmaUnmapMemory(memory::getAllocator(), sGlobalBuffer.allocation);


			// Copy object data to object buffer
			void* objectData;
			vmaMapMemory(memory::getAllocator(), sObjectBuffer.allocation, &objectData);

			ObjectData* objectSSBO = (ObjectData*)objectData;

			/*glm::mat4 rotation = glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(1, 0, 0))*
				glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(0, 1, 0))*
				glm::rotate(glm::mat4{ 1.0f }, glm::radians(mFrameNumber * 0.4f), glm::vec3(0, 0, 1));*/

			for (int i = 0; i < objectCount; i++) {
				RenderObject& object = first[i];
				objectSSBO[i + MAX_OBJECTS * frameIndex].modelMatrix = object.transformMatrix;
				objectSSBO[i + MAX_OBJECTS * frameIndex].inverseModel = glm::inverse(object.transformMatrix);
			}

			vmaUnmapMemory(memory::getAllocator(), sObjectBuffer.allocation);
		}

		void Material::cleanupMaterials() {
			MaterialLayout::cleanupStaticLayouts();
			
			for (auto& entry : sMaterials) {
				Material* mat = entry.second.get();

				mat->cleanup();
			}

			for (auto& entry : sMaterialLayouts) {
				MaterialLayout* mat = entry.second.get();

				mat->cleanup(pDevice);
			}
			
			sGlobalDescriptorPool = nullptr;

			vkDestroySampler(pDevice->getDevice(), sGlobalSampler, nullptr);
		}

		Material* Material::getMaterial(const std::string& name) {
			auto it = sMaterials.find(name);

			if (it == sMaterials.end()) {
				return nullptr;
			}
			else {
				return it->second.get();
			}
		}

		MaterialLayout* Material::getMaterialLayout(const std::string& name) {
			auto it = sMaterialLayouts.find(name);

			if (it == sMaterialLayouts.end()) {
				return nullptr;
			}
			else {
				return it->second.get();
			}
		}
	}
}