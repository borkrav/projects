#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition; // in model space
layout(location = 1) in vec3 inNormal; // in model space

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 normal;

void main() {

    //Final position of the vertex

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4( inPosition, 1.0 );

    fragPos = (ubo.model * vec4( inPosition, 1.0 )).xyz;
    normal = (ubo.model *  vec4(inNormal , 0.0)).xyz;
}