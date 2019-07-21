#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable


struct payload {
	vec3 hitValue;
	vec3 origin;
	vec3 direction;
	bool hit;
	uint seed;
}; 

layout(location = 0) rayPayloadInNV payload pl;
hitAttributeNV vec3 attribs;


struct sphere {
    vec3 centre;
	float radius;
};

layout(binding = 7) readonly buffer SphereData{ sphere [] spheres; };







uint rng_state;

uint rand_xorshift(){
	rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);
    return rng_state;
}

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}







void main()
{


	
	//fetch ray information
	const vec3 origin = gl_WorldRayOriginNV;
	const vec3 direction = gl_WorldRayDirectionNV;
	const float tMin = gl_RayTminNV;
	const float tMax = gl_RayTmaxNV;


	sphere instance = spheres[gl_InstanceCustomIndexNV - 1];

	const vec3 center = instance.centre;
	const float radius = instance.radius;


	const vec3 intersection = origin + gl_HitTNV * direction;
	const vec3 normal = (intersection - center)/radius;


	if (gl_HitTNV > 0.000000000001){

		vec3 dir;

		rng_state = wang_hash(pl.seed);




		do{ 
		

			float val1 = float(rand_xorshift()) * (1.0 / 4294967296.0);
			float val2 = float(rand_xorshift()) * (1.0 / 4294967296.0);
			float val3 = float(rand_xorshift()) * (1.0 / 4294967296.0);
		
			vec3 rng = vec3(val1, val2, val3);

		
			dir = 2.0*rng - vec3(1.0,1.0,1.0);

		} while (dot(dir,dir) >= 1);    
	
		vec3 target = intersection + normal + dir;



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
