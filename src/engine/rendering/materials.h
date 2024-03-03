#pragma once

#include "descriptors.h"
#include "memory/memory_management.h"
#include "renderer.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace engine {
	namespace rendering {

		struct GlobalData {
			glm::vec4 cameraPos;
			glm::vec4 aspect;
			glm::vec4 camDir;

			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
			glm::mat4 invViewProj;
		};

		struct ObjectData {
			glm::mat4 modelMatrix;
			glm::mat4 inverseModel;
		};
		
		enum BindingType {
			BINDING_TYPE_DATA,
			BINDING_TYPE_IMAGE
		};

		struct MaterialLayoutBinding {
			VkDescriptorSetLayoutBinding binding;

			BindingType bindingType;

			size_t bufferSize;
			size_t bufferRange;


			VkImageLayout imageLayout;
			VkSampler* pSampler;
		};

		struct MaterialMemoryObject {
			BindingType bindingType;

			memory::AllocatedBuffer buffer;
			VkDescriptorBufferInfo bufferInfo;

			VkDescriptorImageInfo imageInfo;
		};

		struct MaterialLayout {
			std::string vertFileName;
			std::string fragFileName;

			std::vector<MaterialLayoutBinding> bindings;
			std::unique_ptr<VulkanDescriptorSetLayout> setLayout;

			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;

			MaterialLayout(std::string vertFileName, std::string fragFileName);
			
			//void addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);

			MaterialLayout& addDataBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, size_t bufferSize, size_t bufferRange, uint32_t count = 1);

			MaterialLayout& addImageBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, VkImageLayout imageLayout, uint32_t count = 1);


			void createDescriptorLayout(VulkanDevice* pDevice);
			void createPipeline(VulkanDevice* pDevice, VkRenderPass renderPass);

			MaterialLayout& finalize(VulkanDevice* pDevice, VkRenderPass renderPass);

			void cleanup(VulkanDevice* pDevice);

		
			static void createStaticLayouts(VulkanDevice* pDevice);

			static void cleanupStaticLayouts();
				
			static bool loadShaderModule(VulkanDevice* pDevice, std::string filePath, VkShaderModule* outShaderModule);

			static std::unique_ptr<VulkanDescriptorSetLayout> sGlobalSetLayout;
			static std::unique_ptr<VulkanDescriptorSetLayout> sObjectSetLayout;
		};


		class Material {
		public:
			Material(std::string name, MaterialLayout* layout);

			void bindGlobalSet(VkCommandBuffer cmd, int frameIndex);

			void bindObjectSet(VkCommandBuffer cmd, int frameIndex);

			void bind(VkCommandBuffer cmd, int frameIndex);

			Material& writeBuffer(uint32_t binding, void* data);

			Material& writeImage(uint32_t binding, std::string textureName);

			Material& finalize();


			std::string getName() const { return mName; }

			MaterialMemoryObject& getMemoryObject(uint32_t binding) { return mMemoryObjects[binding]; }


			static Material* create(std::string name, MaterialLayout* layout);

			static MaterialLayout* createMaterialLayout(std::string name, std::string vertexFileName, std::string fragFileName);

			static void initializeMaterials(VulkanDevice* device, VkRenderPass renderPass, int numFrames);

			static void writeGlobalAndObjectData(int frameIndex, int objectCount, RenderObject* first, glm::vec3 camPos, glm::vec3 camRot);

			static void cleanupMaterials();

			static Material* getMaterial(const std::string& name);

			static MaterialLayout* getMaterialLayout(const std::string& name);

			static size_t padUniformBufferSize(size_t originalSize);			
		protected:
			std::string mName;

			MaterialLayout* pMaterialLayout;

			VkDescriptorSet mMaterialDescriptorSet;

			std::vector<MaterialMemoryObject> mMemoryObjects;


			void createMemoryObjects();

			void initDescriptors();

			void cleanup();


			static VkDescriptorSet sGlobalDescriptorSet;
			static memory::AllocatedBuffer sGlobalBuffer;
			
			static std::vector<VkDescriptorSet> sObjectDescriptorSets;
			static memory::AllocatedBuffer sObjectBuffer;
			 
			static std::unique_ptr<VulkanDescriptorPool> sGlobalDescriptorPool;
			 
			static VulkanDevice* pDevice;
			 
			static std::unordered_map<std::string, std::unique_ptr<Material>> sMaterials;
			static std::unordered_map<std::string, std::unique_ptr<MaterialLayout>> sMaterialLayouts;
			
			static VkSampler sGlobalSampler;

			static int sFrameOverlap;
		};
	
		struct BasicData {
			glm::vec4 color;
		};
	}
}