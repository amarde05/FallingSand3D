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

			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
		};

		struct ObjectData {
			glm::mat4 transformMatrix;
		};

		class Material {
		public:
			Material(std::string name, std::string fragName, std::string vertName);


			std::string getName() const { return mName; }

			virtual void writeData() = 0;

			void bind(VkCommandBuffer cmd, int frameIndex);

			static void initializeMaterials(VulkanDevice* device, VkRenderPass renderPass, int numFrames);

			static void createMaterials();

			static void writeGlobalAndObjectData(int frameIndex, int objectCount, RenderObject* first, glm::vec3 camPos);

			static void cleanupMaterials();

			static Material* getMaterial(const std::string& name);
		protected:
			std::string mName;

			std::string mFragFileName;
			std::string mVertFileName;

			std::unique_ptr<VulkanDescriptorSetLayout> mMaterialDescriptorSetLayout;
			VkDescriptorSet mMaterialDescriptorSet;

			VkPipeline mPipeline;
			VkPipelineLayout mPipelineLayout;


			virtual void initDescriptors() = 0;

			virtual void bindMaterialDescriptors(VkCommandBuffer cmd, int frameIndex) = 0;

			virtual void initPipeline(VkRenderPass renderPass);

			virtual void cleanup();

			bool loadShaderModule(std::string filePath, VkShaderModule* outShaderModule) const;

			static size_t padUniformBufferSize(size_t originalSize);
		};
	
		struct BasicData {
			glm::vec4 color;
		};

		class BasicMaterial : Material {
		public:
			BasicMaterial(std::string name, BasicData data);

			virtual void writeData();
		protected:
			BasicData mData;

			memory::AllocatedBuffer mDataBuffer;

			virtual void initDescriptors();
			virtual void bindMaterialDescriptors(VkCommandBuffer cmd, int frameIndex);
		};
	}
}