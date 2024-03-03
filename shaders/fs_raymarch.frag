#version 450

layout (location = 0) in vec3 inFragOrigin;
layout (location = 1) in vec2 inUV;
layout (location = 2) in float inAspect;

layout (location = 0) out vec4 outFragColor;


float sphereSDF(in vec3 p, in vec3 c, float r) {
	return length(p - c) - r;
}

float boxSDF(in vec3 p, in vec3 bounds) {
	vec3 q = abs(p) - bounds;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float map_the_world(in vec3 point) {
	float box1 = boxSDF(point, vec3(0.25));
	
	float sphere1 = sphereSDF(point, vec3(0.0), 0.5);
	
	return sphere1;

}

vec3 calculate_normal(in vec3 point) {
	const vec3 small_step = vec3(0.001, 0.0, 0.0);
	
	float gradient_x = map_the_world(point + small_step.xyy) - map_the_world(point - small_step.xyy);
	float gradient_y = map_the_world(point + small_step.yxy) - map_the_world(point - small_step.yxy);
	float gradient_z = map_the_world(point + small_step.yyx) - map_the_world(point - small_step.yyx);
	
	return normalize(vec3(gradient_x, gradient_y, gradient_z));
}

vec3 ray_march(in vec3 origin, in vec3 dir) {
	float t = 0.0f;
	
	for (int i = 0; i < 64; i++) {
		vec3 current_position = origin + t * dir;
		
		float d = map_the_world(current_position);
		
		if (d < 0.001) {		
			vec3 normal = calculate_normal(current_position);
			
			vec3 light_pos = vec3(2.0, 5.0, 3.0);
			
			vec3 direction_to_light = normalize(current_position - light_pos);
			
			float diffuse_intensity =  max(0.05, dot(normal, direction_to_light));
		
			return vec3(1.0, 0.0, 0.0) * diffuse_intensity;
		}
		else if (t > 1000) {
			break;
		}
		
		t += d;
	}
	
	return vec3(0.0f);
}

void main() {
	vec2 uv = inUV * 2 - 1;
	
	//uv.x *= inAspect;
	uv.y *= 1 / inAspect;
	//uv.x *= inAspect;
	//uv.y *= 1.0 / inAspect;
	
	vec3 dir = normalize(vec3(uv, 1));
	vec3 point = inFragOrigin;
	
	vec3 color = ray_march(point, dir);

	outFragColor = vec4(color, 1.0f);
}