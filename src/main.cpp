#include <pumex/AssetLoaderAssimp.h>
#include <pumex/Pumex.h>
#include <array>
#include <glm/glm.hpp>
#include <string>
#include <vector>

constexpr uint32_t BufferCount = 3U;

struct AssetData {
  enum { Triangle, Cube, Cylinder, IcosphereSub2, Pentagon, COUNT };
  std::array<std::shared_ptr<pumex::Asset>, COUNT> assets;
  std::array<const char*, COUNT> paths = {"content/models/triangle.gltf",
                                          "content/models/cube.gltf",
                                          "content/models/cylinder.gltf",
                                          "content/models/icosphereSub2.gltf",
                                          "content/models/pentagon.gltf"};
};

struct MaterialData {
  enum { Red, Green, Blue, White, Black, COUNT };

  struct Material {
    glm::vec4 color;
  };
  std::array<Material, COUNT> materials;
  std::shared_ptr<pumex::Buffer<std::array<Material, COUNT>>> materialBuffer;
};

struct LightData {
  struct Light {
    glm::vec4 position;
    glm::vec4 color;
  };

  std::vector<Light> lights;
  std::shared_ptr<pumex::Buffer<std::vector<Light>>> lightsBuffer;
};

struct CameraData {
  CameraData(std::shared_ptr<pumex::DeviceMemoryAllocator> buffersAllocator) {
    cameraBuffer = std::make_shared<pumex::Buffer<pumex::Camera>>(
      buffersAllocator,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      pumex::pbPerSurface,
      pumex::swOnce,
      true);
  }
  std::shared_ptr<pumex::Buffer<pumex::Camera>> cameraBuffer;
};

struct InstanceData {
  struct Matrices {
    glm::mat4 model;
    glm::mat4 mvp;
  };
  using InstanceBuffer = std::shared_ptr<pumex::Buffer<std::vector<Matrices>>>;

  InstanceData(
    std::shared_ptr<pumex::DeviceMemoryAllocator> instanceBuffersAllocator) {
    for (auto& buffer : instanceBuffers) {
      buffer = InstanceBuffer::element_type(
        instanceBuffersAllocator,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        pumex::pbPerDevice,
        pumex::swForEachImage,
        true);
    }
  }
  std::array<InstanceBuffer, BufferCount> instanceBuffers;
};

struct ApplicationData {
  AssetData assetData;
  MaterialData materialData;
  LightData lightData;
  CameraData cameraData;
  InstanceData instanceData;
};

int main() {
  std::vector<std::string> requestedInstanceExtensions = {"VK_KHR_surface"};
  std::vector<std::string> requestedDebugLayers = {
    "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_assistant_layer"};
  std::string appName = "PumexApp";
  auto viewerTraits = pumex::ViewerTraits(
    appName, requestedInstanceExtensions, requestedDebugLayers, 60);
  auto viewer = std::make_shared<pumex::Viewer>(viewerTraits);
  auto windowTraits = pumex::WindowTraits{
    0, 50, 50, 900, 900, pumex::WindowTraits::WINDOW, appName};
  auto window = pumex::Window::createWindow(windowTraits);
  std::vector<std::string> requestedExtensions = {"VK_KHR_swapchain"};
  auto device = viewer->addDevice(0, requestedExtensions);
  auto surfaceTraits = pumex::SurfaceTraits(
    BufferCount,
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    1,
    VK_PRESENT_MODE_FIFO_KHR,
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
  viewer->addSurface(window, device, surfaceTraits);

  std::vector<pumex::VertexSemantic> requiredSemantics = {
    {pumex::VertexSemantic::Position, 3}, {pumex::VertexSemantic::Normal, 3}};

  pumex::AssetLoaderAssimp loader;
  auto modelLoadFunc = [&](auto index) {
    assetData.assets[index] =
      loader.load(viewer, assetData.paths[index], false, requiredSemantics);
  };

  for (auto i = 0; i < AssetData::COUNT; ++i) {
    modelLoadFunc(i);
  }

  auto verticesAllocator = std::make_shared<pumex::DeviceMemoryAllocator>(
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    64 * 1024 * 1024,
    pumex::DeviceMemoryAllocator::FIRST_FIT);

  auto frameBufferAllocator = std::make_shared<pumex::DeviceMemoryAllocator>(
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    16 * 1024 * 1024,
    pumex::DeviceMemoryAllocator::FIRST_FIT);

  auto buffersAllocator = std::make_shared<pumex::DeviceMemoryAllocator>(
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    16 * 1024 * 1024,
    pumex::DeviceMemoryAllocator::FIRST_FIT);

  std::vector<pumex::QueueTraits> queueTraits{
    {VK_QUEUE_GRAPHICS_BIT, 0, 0.75f}};

  std::shared_ptr<pumex::RenderWorkflow> workflow =
    std::make_shared<pumex::RenderWorkflow>(
      "viewer_workflow", frameBufferAllocator, queueTraits);

  workflow->addResourceType(
    "depthBuffer",
    false,
    VK_FORMAT_D32_SFLOAT,
    VK_SAMPLE_COUNT_1_BIT,
    pumex::atDepth,
    pumex::AttachmentSize{pumex::AttachmentSize::SurfaceDependent,
                          glm::vec2(1.f)},
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

  workflow->addResourceType(
    "surface",
    true,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_SAMPLE_COUNT_1_BIT,
    pumex::atSurface,
    pumex::AttachmentSize{pumex::AttachmentSize::SurfaceDependent,
                          glm::vec2(1.f)},
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

  workflow->addRenderOperation("Ortho3D", pumex::RenderOperation::Graphics);

  workflow->addAttachmentDepthOutput(
    "Ortho3D",
    "depthBuffer",
    "depthOutput",
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    pumex::loadOpClear(glm::vec2(1.0f, 0.0f)));

  workflow->addAttachmentOutput(
    "Ortho3D",
    "surface",
    "colorOutput",
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    pumex::loadOpClear(glm::vec4(0.f)));

  auto ortho3Droot = std::make_shared<pumex::Group>();
  ortho3Droot->setName("ortho3Droot");
  workflow->setRenderOperationNode("Ortho3D", ortho3Droot);

  std::vector<pumex::DescriptorSetLayoutBinding> staticBindings = {
    {2,
     MaterialData::COUNT,
     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     VK_SHADER_STAGE_FRAGMENT_BIT},
    {3, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
    {4,
     LightData::COUNT,
     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT}};
  auto staticSetLayout =
    std::make_shared<pumex::DescriptorSetLayout>(staticBindings);

  std::vector<pumex::DescriptorSetLayoutBinding> dynamicBindings = {
    {0,
     1,
     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
     VK_SHADER_STAGE_VERTEX_BIT}};
  auto dynamicSetLayout =
    std::make_shared<pumex::DescriptorSetLayout>(dynamicBindings);

  auto pipelineLayout = std::make_shared<pumex::PipelineLayout>();
  pipelineLayout->descriptorSetLayouts = {staticSetLayout, dynamicSetLayout};

  auto pipelineCache = std::make_shared<pumex::PipelineCache>();

  auto pipeline =
    std::make_shared<pumex::GraphicsPipeline>(pipelineCache, pipelineLayout);
  pipeline->shaderStages = {
    {VK_SHADER_STAGE_VERTEX_BIT,
     std::make_shared<pumex::ShaderModule>(viewer, "shaders/3D/vert.spv"),
     "main"},
    {VK_SHADER_STAGE_FRAGMENT_BIT,
     std::make_shared<pumex::ShaderModule>(viewer, "shaders/3D/frag.spv"),
     "main"}};

  pipeline->vertexInput = {{0, VK_VERTEX_INPUT_RATE_VERTEX, requiredSemantics}};

  pipeline->blendAttachments = {{VK_FALSE, 0xF}};

  ortho3Droot->addChild(pipeline);

  for (auto i = 0U; i < AssetData::COUNT; ++i) {
    auto assetNode = std::make_shared<pumex::AssetNode>(assetData.assets[i]);
    pipeline->addChild(assetNode);
  }

  auto cameraData = std::make_shared<CameraData>(buffersAllocator);
  auto instanceData = std::make_shared<InstanceData>(buffersAllocator);

  viewer->run();
  return 0;
}