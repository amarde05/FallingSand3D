#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) noperspective out vec3 outFragOrigin;
layout (location = 1) noperspective out vec3 outFragLocalPos;

layout (set = 0, binding = 0) uniform GlobalData{
	vec4 camPos;
	vec4 aspect;
	vec4 camDir;
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
	mat4 model = objectBuffer.objects[gl_BaseInstance].model;
	mat4 invModel = objectBuffer.objects[gl_BaseInstance].invModel;

	mat4 transformMatrix = globalData.viewproj * model;
	
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	

	vec4 camPos = vec4(globalData.camPos.xyz * -1, 1);

	// Find the camera position in object space
	vec4 localCamPos = (invModel * camPos);
	
	outFragOrigin = localCamPos.xyz;
	outFragLocalPos = vPosition;
	
	//float d = dot(globalData.camDir.xyz, vNormal);
	
	//if (d == 1)
	//	outFragLocalPos.z *= -1;
	//else {
	//	outFragLocalPos.z *= -1;
	//}
	
	//if (vNormal == vec3(0, 0, 1))
	
	//outFragWorldPos.y *= -1;
	//outFragWorldPos.x *= -1;
}