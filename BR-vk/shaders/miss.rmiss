#version 460
#extension GL_EXT_ray_tracing : enable

struct payload {
	vec3 hitValue;
	vec3 origin;
	vec3 direction;
	bool hit;
	uint seed;
	uint mode;
}; 

layout(location = 0) rayPayloadInEXT payload rayResult;

void main()
{
    const vec3 direction = gl_WorldRayDirectionEXT;
	float t = 0.5*(normalize(direction).y+1.0);

    rayResult.hitValue = (1.0-t)*vec3(1.0) + t*vec3(0.5,0.7,1.0);
    rayResult.hit = false;
}