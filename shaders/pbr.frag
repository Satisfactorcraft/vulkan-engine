#version 450
#define PI 3.14159265359
#define MAX_LIGHTS 4

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in mat3 inTBN;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec4 camPos;
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    int  numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D texAlbedo;
layout(set = 0, binding = 2) uniform sampler2D texNormal;
layout(set = 0, binding = 3) uniform sampler2D texMetalRough;
layout(set = 0, binding = 4) uniform sampler2D texAO;

layout(location = 0) out vec4 outColor;

float D_GGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float G_SchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3  albedo    = pow(texture(texAlbedo, inUV).rgb, vec3(2.2));
    vec2  mr        = texture(texMetalRough, inUV).bg;
    float metallic  = mr.x;
    float roughness = clamp(mr.y, 0.04, 1.0);
    float ao        = texture(texAO, inUV).r;

    vec3 N = texture(texNormal, inUV).rgb * 2.0 - 1.0;
    N = normalize(inTBN * N);

    vec3 V  = normalize(ubo.camPos.xyz - inWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < ubo.numLights; ++i) {
        vec3  L        = normalize(ubo.lightPos[i].xyz - inWorldPos);
        vec3  H        = normalize(V + L);
        float dist     = length(ubo.lightPos[i].xyz - inWorldPos);
        float atten    = 1.0 / (dist * dist);
        vec3  radiance = ubo.lightColor[i].rgb * ubo.lightColor[i].w * atten;

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        vec3  F = F_Schlick(HdotV, F0);

        vec3 kD       = (1.0 - F) * (1.0 - metallic);
        vec3 diffuse  = kD * albedo / PI;
        vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);

        Lo += (diffuse + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color   = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
