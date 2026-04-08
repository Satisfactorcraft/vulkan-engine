#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec4 camPos;
    vec4 lightPos[4];
    vec4 lightColor[4];
    int  numLights;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out mat3 outTBN;

void main() {
    vec4 worldPos = model * vec4(inPos, 1.0);
    outWorldPos   = worldPos.xyz;
    outUV         = inUV;

    // TBN-Matrix für Normal Mapping
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 T = normalize(normalMatrix * inTangent);
    T = normalize(T - dot(T, N) * N);   // Gram-Schmidt
    vec3 B = cross(N, T);
    outTBN = mat3(T, B, N);

    gl_Position = ubo.proj * ubo.view * worldPos;
}
