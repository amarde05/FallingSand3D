#version 450

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 2, binding = 0) uniform BasicData {
	vec4 color;
} basicData;

layout (set = 2, binding = 1) uniform sampler2D tex1;

void main() {
	vec3 color = texture(tex1, inTexCoord).xyz;

	outFragColor = basicData.color + vec4(color, 1.0f);
}