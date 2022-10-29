#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "rng.glsl"

struct payload {
	vec3 hitValue;
	vec3 origin;
	vec3 direction;
	bool hit;
	uint seed;
}; 

layout(location = 0) rayPayloadInEXT payload rayResult;


layout(binding = 3, set = 0 ) buffer vertices
{
  vec4 v[];
};
layout(binding = 4, set = 0) buffer indices
{
  uint i[];
};

// barycentric weights of the intersection point
hitAttributeEXT vec2 attribs;

void main()
{
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

  //Primitive ID - what got hit
  int primitiveID = gl_PrimitiveID;

  //Fetch the 3 indices of the triagle
  const uint i0 = i[3*primitiveID + 0];
  const uint i1 = i[3*primitiveID + 1];
  const uint i2 = i[3*primitiveID + 2];

  //Fetch the 3 vertices of the triangle
  const vec3 v0 = v[i0].xyz;
  const vec3 v1 = v[i1].xyz;
  const vec3 v2 = v[i2].xyz;


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


  // Compute the position of the intersection in object space
  const vec3 objectSpaceIntersection = barycentricCoords.x * v0 + barycentricCoords.y * v1 + barycentricCoords.z * v2;

  // Intersection position in world space
  const vec3 worldSpaceIntersection = gl_ObjectToWorldEXT  * vec4(objectSpaceIntersection, 1);

  vec3 objectNormal = normalize(cross(v1 - v0, v2 - v0));
  objectNormal = gl_ObjectToWorldEXT  * vec4(objectNormal, 0);

  vec3 originalVector = gl_WorldRayDirectionEXT - gl_WorldRayOriginEXT;
  vec3 bounce = reflect(originalVector, objectNormal.xyz);


  // Mirror Reflections
  vec3 target = worldSpaceIntersection + bounce /*+ dir*/;

  //Occlusion
  //vec3 target = worldSpaceIntersection + objectNormal /*+ dir*/;

  //vec3 target = worldSpaceIntersection + objectNormal + dir;

  // Return it as a color

  if(gl_HitTEXT > 1e-15)
  {
    rayResult.hitValue = vec3(0.5);
    rayResult.origin = worldSpaceIntersection;
    rayResult.direction = target - worldSpaceIntersection;
    rayResult.hit = true;
 }

  else 
  {
		rayResult.hitValue = vec3(0.5);
		rayResult.hit = false;
	}

}