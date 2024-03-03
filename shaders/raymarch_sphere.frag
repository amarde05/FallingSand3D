#version 450

layout (location = 0) noperspective in vec3 inFragOrigin;
layout (location = 1) noperspective in vec3 inFragLocalPos;

layout (location = 0) out vec4 outFragColor;


float sphereSDF(in vec3 p, float r) {
	return length(p) - r;
}

float boxSDF(in vec3 p, in vec3 bounds) {
	vec3 q = abs(p) - bounds;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

vec3 ray_march(in vec3 origin, in vec3 dir) {
	float t = 0.0;
	
	float density = 1.0;
	
	for (int i = 0; i < 64; i++) {
		vec3 current_position = origin + dir * t;
		
		float d = boxSDF(current_position, vec3(0.5));
		//float d = sphereSDF(current_position, 0.5);
		
		if (d < 0.001) {		
			return vec3(max(0.05, density), 0.0f, 0.0f);
		}
		else if (t > 1000) {
			break;
		}
		
		t += d;
		
		density -= 0.05;
	}
	
	return vec3(0);
}

void main() {
	vec3 dir = normalize(inFragLocalPos - inFragOrigin);
	vec3 point = inFragOrigin;
	
	vec3 color = ray_march(point, dir);

	outFragColor = vec4(color, 1);
}