#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "rng.glsl"

hitAttributeNV vec3 attribs;

struct payload {
	vec3 hitValue;
	vec3 origin;
	vec3 direction;
	bool hit;
	uint seed;
}; 

layout(location = 0) rayPayloadInNV payload pl;


struct sphere {
    vec3 centre;
	float radius;
};

layout(binding = 7) readonly buffer SphereData{ sphere [] spheres; };




void main()
{


	
	//fetch ray information
	const vec3 origin = gl_WorldRayOriginNV;
	const vec3 direction = gl_WorldRayDirectionNV;
	const float tMin = gl_RayTminNV;
	const float tMax = gl_RayTmaxNV;


	//fetch sphere information
	sphere instance = spheres[gl_InstanceCustomIndexNV - 1];
	const vec3 center = instance.centre;
	const float radius = instance.radius;


	const vec3 intersection = origin + gl_HitTNV * direction;
	const vec3 normal = (intersection - center)/radius;

	rng_state = pl.seed;

	//epsilon check
	if (gl_HitTNV > 1e-15){

		vec3 dir;

		//this is for safety - to avoid infinite loop
		int count = 0;
		
		do{ 
		

			float val1 = float(rand_xorshift()) * (1.0 / 4294967296.0);
			float val2 = float(rand_xorshift()) * (1.0 / 4294967296.0);
			float val3 = float(rand_xorshift()) * (1.0 / 4294967296.0);
		
			vec3 rng = vec3(val1, val2, val3);

			dir = 2.0*rng - vec3(1.0,1.0,1.0);

			count += 1;

		} while (dot(dir,dir) >= 1 && count < 1000);    
	
		vec3 target = intersection + normal + dir;

		//write information for next bounce
		pl.hitValue = vec3(0.5);
		pl.origin = intersection;
		pl.direction = target-intersection;
		pl.hit = true;
		pl.seed = rng_state;
	}
	
	else {
		pl.hitValue = vec3(0.5);
		pl.hit = false;
	}
	
}
