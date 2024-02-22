#version 450

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform BasicData {
	vec4 color;
} basicData;

void main() {
	outFragColor = basicData.color;
}