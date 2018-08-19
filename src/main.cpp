#include <pumex/AssetLoaderAssimp.h>
#include <pumex/Pumex.h>
#include <tbb/tbb.h>
#include <array>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

constexpr uint32_t BufferCount = 3U;
constexpr uint32_t PrimaryRenderMask = 1U;

struct AssetData {
  enum AssetID { Triangle, Cube, Cylinder, IcosphereSub2, Pentagon, COUNT };
  pumex::AssetLoaderAssimp loader;
  std::array<std::shared_ptr<pumex::Asset>, COUNT> assets;
  std::array<const char*, COUNT> paths = {"content/models/triangle.blend",
                                          "content/models/cube.blend",
                                          "content/models/cylinder.blend",
                                          "content/models/icosphereSub2.blend",
                                          "content/models/pentagon.blend"};

  std::array<pumex::DrawIndexedIndirectCommand, COUNT> drawCmds;
  std::shared_ptr<
    pumex::Buffer<std::array<pumex::DrawIndexedIndirectCommand, COUNT>>>
    drawCmdBuffer;
  std::shared_ptr<pumex::AssetBuffer> assetBuffer;

  AssetData(
    std::shared_ptr<pumex::DeviceMemoryAllocator> bufferAllocator,
    std::shared_ptr<pumex::DeviceMemoryAllocator> vertexAllocator,
    std::shared_ptr<pumex::Viewer> viewer,
    const std::vector<pumex::VertexSemantic>& requiredSemantic) {
    std::vector<pumex::AssetBufferVertexSemantics> semantics{
      {PrimaryRenderMask, requiredSemantic}};
    assetBuffer = std::make_shared<pumex::AssetBuffer>(
      semantics, bufferAllocator, vertexAllocator);

    for (auto i = 0U; i < COUNT; ++i) {
      assets[i] = loader.load(viewer, paths[i], false, requiredSemantic);
      const auto& asset = *assets[i];
      auto tdef = pumex::AssetTypeDefinition(
        pumex::calculateBoundingBox(asset, PrimaryRenderMask));
      assetBuffer->registerType(i, tdef);
      auto ldef = pumex::AssetLodDefinition();
      assetBuffer->registerObjectLOD(i, ldef, assets[i]);
    }
  }
};

struct MaterialData {
  enum { Red, Green, Blue, White, Black, COUNT };

  struct Material {
    glm::vec4 color;

    void registerProperties(const pumex::Material& material) {}

    void registerTextures(
      const std::map<pumex::TextureSemantic::Type, uint32_t>& textureIndices) {}
  };
  std::array<Material, COUNT> materials = {glm::vec4{1.f, 0.f, 0.f, 1.f},
                                           glm::vec4{0.f, 1.f, 0.f, 1.f},
                                           glm::vec4{0.f, 0.f, 1.f, 1.f},
                                           glm::vec4{1.f, 1.f, 1.f, 1.f},
                                           glm::vec4{0.f, 0.f, 0.f, 1.f}};
  using MaterialBuffer = pumex::Buffer<std::array<Material, COUNT>>;
  std::shared_ptr<MaterialBuffer> materialBuffer;

  MaterialData(std::shared_ptr<pumex::DeviceMemoryAllocator> bufferAllocator) {
    materialBuffer = std::make_shared<MaterialBuffer>(
      std::make_shared<std::array<Material, COUNT>>(materials),
      bufferAllocator,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      pumex::pbPerDevice,
      pumex::swOnce);
  }
};

struct LightData {
  static constexpr auto MaxLights = 3U;
  struct Lights {
    struct Light {
      glm::vec4 position;
      glm::vec4 color;
    } lightArray[MaxLights];
    uint32_t LightCount;
  } lights;

  std::shared_ptr<pumex::Buffer<Lights>> lightsBuffer;

  LightData(std::shared_ptr<pumex::DeviceMemoryAllocator> bufferAllocator) {
    lightsBuffer = std::make_shared<pumex::Buffer<Lights>>(
      bufferAllocator,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      pumex::pbPerDevice,
      pumex::swForEachImage,
      true);
  }
};

struct CameraData {
  using CameraBuffer = pumex::Buffer<pumex::Camera>;
  std::shared_ptr<CameraBuffer> cameraBuffer;

  CameraData(std::shared_ptr<pumex::DeviceMemoryAllocator> bufferAllocator) {
    cameraBuffer = std::make_shared<pumex::Buffer<pumex::Camera>>(
      bufferAllocator,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      pumex::pbPerDevice,
      pumex::swForEachImage,
      true);
  }
};

struct InstanceData {
  struct Matrices {
    glm::mat4 model;
    glm::mat4 mvp;
    uint32_t materialIndex;
    uint32_t textureIndex;
    uint32_t padding0;
    uint32_t padding1;
  };
  std::array<std::vector<Matrices>, BufferCount> matrices;
  using InstanceBuffer = pumex::Buffer<std::vector<Matrices>>;
  std::shared_ptr<InstanceBuffer> instanceBuffer;

  InstanceData(std::shared_ptr<pumex::DeviceMemoryAllocator> bufferAllocator) {
    instanceBuffer = std::make_shared<InstanceBuffer>(
      bufferAllocator,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      pumex::pbPerDevice,
      pumex::swForEachImage,
      true);
  }
};

struct ApplicationData {
  std::shared_ptr<AssetData> assetData;
  std::shared_ptr<MaterialData> materialData;
  std::shared_ptr<LightData> lightData;
  std::shared_ptr<CameraData> cameraData;
  std::shared_ptr<InstanceData> instanceData;

  ApplicationData(
    std::shared_ptr<pumex::DeviceMemoryAllocator> bufferAllocator,
    std::shared_ptr<pumex::DeviceMemoryAllocator> vertexAllocator,
    std::shared_ptr<pumex::Viewer> viewer,
    const std::vector<pumex::VertexSemantic>& requiredSemantic) {
    assetData = std::make_shared<AssetData>(
      bufferAllocator, vertexAllocator, viewer, requiredSemantic);
    materialData = std::make_shared<MaterialData>(bufferAllocator);
    lightData = std::make_shared<LightData>(bufferAllocator);
    cameraData = std::make_shared<CameraData>(bufferAllocator);
    instanceData = std::make_shared<InstanceData>(bufferAllocator);
  }

  void update(
    std::shared_ptr<pumex::Viewer> viewer,
    double updateBeginTime,
    double updateDuration) {}
  void renderStart() {}
  void surfaceRenderStart() {}
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
  auto surface = viewer->addSurface(window, device, surfaceTraits);

  std::vector<pumex::VertexSemantic> requiredSemantic = {
    {pumex::VertexSemantic::Position, 3}, {pumex::VertexSemantic::Normal, 3}};

  constexpr auto MegaBytes = 1024U * 1024U;
  auto vertexAllocator = std::make_shared<pumex::DeviceMemoryAllocator>(
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    16 * MegaBytes,
    pumex::DeviceMemoryAllocator::FIRST_FIT);

  auto frameBufferAllocator = std::make_shared<pumex::DeviceMemoryAllocator>(
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    16 * MegaBytes,
    pumex::DeviceMemoryAllocator::FIRST_FIT);

  auto bufferAllocator = std::make_shared<pumex::DeviceMemoryAllocator>(
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    16 * MegaBytes,
    pumex::DeviceMemoryAllocator::FIRST_FIT);

  auto appData = std::make_shared<ApplicationData>(
    bufferAllocator, vertexAllocator, viewer, requiredSemantic);

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

  auto materialBinding = pumex::DescriptorSetLayoutBinding{
    2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT};
  auto cameraBinding = pumex::DescriptorSetLayoutBinding{
    3, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT};
  auto lightBinding = pumex::DescriptorSetLayoutBinding{
    4,
    1,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};
  auto instanceBinding = pumex::DescriptorSetLayoutBinding{
    5,
    1,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};

  std::vector<pumex::DescriptorSetLayoutBinding> bindings = {
    materialBinding, cameraBinding, lightBinding, instanceBinding};
  auto setLayout = std::make_shared<pumex::DescriptorSetLayout>(bindings);

  auto pipelineLayout = std::make_shared<pumex::PipelineLayout>();
  pipelineLayout->descriptorSetLayouts.push_back(setLayout);

  auto pipelineCache = std::make_shared<pumex::PipelineCache>();

  auto pipeline =
    std::make_shared<pumex::GraphicsPipeline>(pipelineCache, pipelineLayout);
  pipeline->shaderStages = {{VK_SHADER_STAGE_VERTEX_BIT,
                             std::make_shared<pumex::ShaderModule>(
                               viewer, "shaders/3D/shader.vert.spv"),
                             "main"},
                            {VK_SHADER_STAGE_FRAGMENT_BIT,
                             std::make_shared<pumex::ShaderModule>(
                               viewer, "shaders/3D/shader.frag.spv"),
                             "main"}};

  pipeline->vertexInput = {{0, VK_VERTEX_INPUT_RATE_VERTEX, requiredSemantic}};

  pipeline->blendAttachments = {{VK_FALSE, 0xF}};

  ortho3Droot->addChild(pipeline);

  auto materialRegistry =
    std::make_shared<pumex::MaterialRegistry<MaterialData::Material>>(
      bufferAllocator);

  auto textureRegistry = std::make_shared<pumex::TextureRegistryNull>();

  auto textureSemantic = std::vector<pumex::TextureSemantic>{};
  auto assetBufferNode = std::make_shared<pumex::AssetBufferNode>(
    appData->assetData->assetBuffer,
    std::make_shared<pumex::MaterialSet>(
      viewer,
      materialRegistry,
      textureRegistry,
      bufferAllocator,
      textureSemantic),
    PrimaryRenderMask,
    0);

  pipeline->addChild(assetBufferNode);

  auto workflowCompiler =
    std::make_shared<pumex::SingleQueueWorkflowCompiler>();
  surface->setRenderWorkflow(workflow, workflowCompiler);

  tbb::flow::continue_node<tbb::flow::continue_msg> updateNode(
    viewer->updateGraph, [=](tbb::flow::continue_msg) {
      appData->update(
        viewer,
        pumex::inSeconds(
          viewer->getUpdateTime() - viewer->getApplicationStartTime()),
        pumex::inSeconds(viewer->getUpdateDuration()));
    });

  tbb::flow::make_edge(viewer->opStartUpdateGraph, updateNode);
  tbb::flow::make_edge(updateNode, viewer->opEndUpdateGraph);

  viewer->setEventRenderStart(
    [=](pumex::Viewer* viewer) { appData->renderStart(); });

  surface->setEventSurfaceRenderStart(
    [=](std::shared_ptr<pumex::Surface> surface) {});

  viewer->run();
  return 0;
}