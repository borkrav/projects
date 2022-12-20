#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(binding = 1) buffer instancePositions {
    mat4 pos[2000];
} instancePos;

layout(binding = 2) buffer instanceColors {
    vec4 col[2000];
} instanceCols;


layout(location = 0) in vec3 inPosition; // in model space
layout(location = 1) in vec3 inNormal; // in model space

layout(location = 0) out vec3 outColor;

void main() {

    vec3 lightPos = vec3( 10, 10, 0 ); //in model space
    vec3 objColor = vec3( instanceCols.col[gl_InstanceIndex].xyz ); 
    vec3 ambient = vec3( 0.3, 0.3, 0.3);

    // the position of the light, in view space
    vec4 light_pos_view = ubo.view * vec4( lightPos, 1 ); 

    // the vector from light to surface, in view space
    vec4 light_vec_view = light_pos_view - ( ubo.view * instancePos.pos[gl_InstanceIndex] * vec4( inPosition, 1.0 ) );

    // the surface normal, in view space
    vec4 normal_view = ubo.view * instancePos.pos[gl_InstanceIndex] * vec4( inNormal, 0 );

    // normalized light vector
    vec4 light_vec_view_n = normalize( light_vec_view );

    // dot product between the normal and the light vector
    // perpendicular - 1
    // parallel - 0
    float cosTheta = clamp( dot( normal_view, light_vec_view_n ), 0, 1 );

    // position of camera, in view space
    vec4 eye_pos_view = ubo.view * vec4( ubo.cameraPos, 1 );

    // vector from camera to surface, in view space
    vec4 eye_vec_view = eye_pos_view - ( ubo.view * instancePos.pos[gl_InstanceIndex] * vec4( inPosition, 1.0 ) ) ;

    // normalized eye vector
    vec4 eye_vec_view_n = normalize( eye_vec_view );

    // reflect the light vector around the normal
    vec4 refl = reflect( -light_vec_view_n, normal_view );

    // dot product between the eye vector and the reflected light vector
    float cosAlpha = clamp( dot( eye_vec_view_n, refl ), 0, 1 );

    gl_Position = ubo.proj * ubo.view * instancePos.pos[gl_InstanceIndex] * vec4( inPosition, 1.0 );
    outColor = ambient*objColor + objColor * cosTheta + objColor * pow( cosAlpha, 5 );
}