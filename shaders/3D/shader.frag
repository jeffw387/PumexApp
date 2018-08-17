#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
// ---Fragment Shader 3D pipeline---

// --uniform buffers--
// per material
layout(std430, set = 0, binding = 2) readonly buffer Materials
{
    vec4 color;
} materials[];

// per frame
struct Light {
  vec4 position;
  vec4 color;
};
const uint MaxLights = 3;
layout(set = 0, binding = 4) uniform Lights {
  Light lightArray[MaxLights];
  uint LightCount;
} lights;

// --shader interface--
layout(location = 0) in vec3 normal_CameraSpace;
layout(location = 1) in flat uint materialIndex;
layout(location = 2) in flat uint textureIndex;
layout(location = 3) in vec3 lightDirection_CameraSpace[MaxLights];

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 diffuseColor = materials[materialIndex].color;
    for (uint i = 0; i < lights.LightCount; i++)
    {
        Light currentLight = lights.lightArray[i];
        vec3 l = normalize(lightDirection_CameraSpace[i]);
        vec3 n = normalize(normal_CameraSpace);
        float cosTheta = clamp(dot(n, l), 0, 1);
        diffuseColor *= vec4(currentLight.color.rgb * (cosTheta * currentLight.color.a), 1);
    }
    outColor = diffuseColor;
}