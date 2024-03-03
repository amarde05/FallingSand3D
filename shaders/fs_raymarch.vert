#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec3 outFragOrigin;
layout (location = 1) out vec2 outUV;
layout (location = 2) out float outAspect;

layout (set = 0, binding = 0) uniform GlobalData{
	vec4 camPos;
	vec4 aspect;
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	mat4 invViewProj;
} globalData;

struct ObjectData{
	mat4 model;
	mat4 invModel;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

void main() {
	gl_Position = vec4(vPosition, 1.0f);

	outFragOrigin = globalData.camPos.xyz;
	outUV = vTexCoord;
	
	outAspect = globalData.aspect.x;
}