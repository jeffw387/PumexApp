#version 450
#extension GL_ARB_separate_shader_objects : enable
// ---Vertex Shader 3D pipeline---

// --vertex attributes--
layout(location = 0) in vec3 vertexPosition_ModelSpace;
layout(location = 1) in vec3 vertexNormal_ModelSpace;

// --uniform buffers--
// per frame
layout(set = 0, binding = 3) uniform Camera
{
  mat4 view;
  mat4 projection;
} camera;

struct Light {
  vec4 position;
  vec4 color;
};
const uint MaxLights = 3;
layout(set = 0, binding = 4) uniform Lights {
  Light lightArray[MaxLights];
  uint LightCount;
} lights;

// per instance
struct InstanceData {
  mat4 M;
  mat4 MVP;
  uint materialIndex;
  uint textureIndex;
  uint padding0;
  uint padding1;
};

layout(std430, set = 0, binding = 5) readonly buffer Instance
{
  InstanceData data[];
} instance;

// --shader interface--
layout(location = 0) out vec3 normal_CameraSpace;
layout(location = 1) out vec3 lightDirection_CameraSpace[MaxLights];
out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  InstanceData instanceData = instance.data[gl_InstanceIndex];
  mat4 MVP = instanceData.MVP;
  mat4 M = instanceData.M;
  mat4 V = camera.view;
  mat4 P = camera.projection;

  gl_Position = MVP * vec4(vertexPosition_ModelSpace, 1.0);
    
  vec3 vertexPosition_CameraSpace = 
    (V * M * vec4(vertexPosition_ModelSpace, 1)).xyz;
  vec3 eyePosition_CameraSpace = 
    vec3(0, 0, 0) - vertexPosition_CameraSpace;

  for (uint i = 0; i < lights.LightCount; i++)
  {
    vec3 lightPosition_CameraSpace = 
      (V * vec4(lights.lightArray[i].position)).xyz;
    lightDirection_CameraSpace[i] = 
      lightPosition_CameraSpace + eyePosition_CameraSpace;
  }

  normal_CameraSpace = (V * M * vec4(vertexNormal_ModelSpace, 0)).xyz;
}