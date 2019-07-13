#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInNV vec3 hitValue;
hitAttributeNV vec3 attribs;

void main()
{

	//fetch ray information
	const vec3 origin = gl_WorldRayOriginNV;
	const vec3 direction = gl_WorldRayDirectionNV;
	const float tMin = gl_RayTminNV;
	const float tMax = gl_RayTmaxNV;

	//for now, hardcoded sphere information
	const vec3 center = vec3(0,2,0);
	const float radius = 0.5;

	const vec3 intersection = origin + gl_HitTNV * direction;
	const vec3 normal = intersection - center;

	const vec3 unit = normalize(normal);

	hitValue = unit;
}
