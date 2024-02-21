#include "mesh.h"
#include "../../util/debug.h"

#include <tiny_obj_loader.h>
#include <iostream>

namespace engine {
	namespace rendering {
		VertexInputDescription Vertex::getVertexDescription() {
			VertexInputDescription description;

			// Just 1 vertex buffer binding, with a per-vertex rate
			VkVertexInputBindingDescription mainBinding{};
			mainBinding.binding = 0;
			mainBinding.stride = sizeof(Vertex);
			mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			description.bindings.push_back(mainBinding);

			// Position will be stored at location 0
			VkVertexInputAttributeDescription positionAttrib{};
			positionAttrib.binding = 0;
			positionAttrib.location = 0;
			positionAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
			positionAttrib.offset = offsetof(Vertex, position);

			// Normal will be stored at location 1
			VkVertexInputAttributeDescription normalAttrib{};
			normalAttrib.binding = 0;
			normalAttrib.location = 1;
			normalAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
			normalAttrib.offset = offsetof(Vertex, normal);

			// Color will be stored at location 2
			VkVertexInputAttributeDescription colorAttrib{};
			colorAttrib.binding = 0;
			colorAttrib.location = 2;
			colorAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttrib.offset = offsetof(Vertex, color);

			// UV will be stored at location 3
			VkVertexInputAttributeDescription uvAttrib{};
			uvAttrib.binding = 0;
			uvAttrib.location = 3;
			uvAttrib.format = VK_FORMAT_R32G32_SFLOAT;
			uvAttrib.offset = offsetof(Vertex, uv);

			description.attributes.push_back(positionAttrib);
			description.attributes.push_back(normalAttrib);
			description.attributes.push_back(colorAttrib);
			description.attributes.push_back(uvAttrib);

			return description;
		}


		bool Mesh::loadFromObj(const char* filename) {
			tinyobj::attrib_t attrib;

			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			std::string warn;
			std::string err;

			tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, "../../assets/");

			if (!warn.empty()) {
				util::displayMessage(warn, DISPLAY_TYPE_WARN);
			}

			if (!err.empty()) {
				util::displayMessage(err, DISPLAY_TYPE_NONE);

				//return false;
			}


			// Loop over shapes
			for (size_t s = 0; s < shapes.size(); s++) {
				size_t indexOffset = 0;

				// Loop over faces
				for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
					// Hardcode loading to trinagles
					int fv = 3;

					// Loop over vertices in the face
					for (size_t v = 0; v < fv; v++) {
						tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];

						Vertex newVert{};

						tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
						tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
						tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

						newVert.position.x = vx;
						newVert.position.y = vy;
						newVert.position.z = vz;

						if (attrib.normals.size() > 0) {
							tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
							tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
							tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
							
							newVert.normal.x = nx;
							newVert.normal.y = ny;
							newVert.normal.z = nz;
						}

						if (attrib.texcoords.size() > 0) {
							tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
							tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];

							newVert.uv.x = ux;
							newVert.uv.y = 1 - uy;
						}

						// Set vertex color to normal for display
						newVert.color = newVert.normal;

						vertices.push_back(newVert);
					}

					indexOffset += fv;
				}
			}

			return true;
		}
	}
}