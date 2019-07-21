#version 460
#extension GL_NV_ray_tracing : require

struct payload {
	vec3 hitValue;
	vec3 origin;
	vec3 direction;
	bool hit;
}; 

layout(location = 0) rayPayloadInNV payload pl;

void main()
{

	const vec3 direction = gl_WorldRayDirectionNV;
	float t = 0.5*(normalize(direction).y+1.0);

    pl.hitValue = (1.0-t)*vec3(1.0) + t*vec3(0.5,0.7,1.0);
	pl.hit = false;
}