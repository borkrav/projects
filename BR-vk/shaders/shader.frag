#version 450

layout(location = 0) in smooth vec3 fragPos;
layout(location = 1) in smooth vec3 normal;

layout(location = 0) out vec4 outColor;


void main() {


    vec3 lightPos = vec3(0,10,0);

    vec3 lightVec = normalize(lightPos - fragPos);
    vec3 norm = normalize(normal);


    float diffuse = clamp(dot(norm, lightVec), 0.0, 1.0);

    vec3 diff = diffuse * vec3(1,1,1);
     
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    vec3 objColor = vec3(1,0.5,0);

    outColor = vec4((diff + ambient) * objColor, 1.0);
}