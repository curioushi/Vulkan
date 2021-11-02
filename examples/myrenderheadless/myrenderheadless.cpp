#include <cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <string>
#include <algorithm>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define CHECK_VK_SUCCESS(ret) \
  if ((ret) != VK_SUCCESS) {  \
    std::cerr << "check vk success failed at line: " << __LINE__; \
  }

VkVertexInputBindingDescription initializeVertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
  VkVertexInputBindingDescription description;
  description.binding = binding;
  description.stride = stride;
  description.inputRate = inputRate;
  return description;
}

VkVertexInputAttributeDescription initializeVertexInputAttribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) {
  VkVertexInputAttributeDescription description;
  description.binding = binding;
  description.location = location;
  description.format = format;
  description.offset = offset;
  return description;
}

VkPushConstantRange initializePushConstanceRange(uint32_t size, uint32_t offset, VkShaderStageFlags stageFlags) {
  VkPushConstantRange constantRange;
  constantRange.size = size;
  constantRange.offset = offset;
  constantRange.stageFlags = stageFlags;
  return constantRange;
}

class HeadlessRenderer {
 public:
  VkInstance instance_;
  VkPhysicalDevice physicalDevice_;
  uint32_t queueFamilyIndex_ = -1;
  VkDevice device_;
  VkQueue queue_;
  VkCommandPool commandPool_;

  VkBuffer vertexBuffer_;
  VkDeviceMemory vertexMemory_;

  VkBuffer indexBuffer_;
  VkDeviceMemory indexMemory_;

  VkFormat colorFormat_;
  VkImage color_;
  VkImageView colorView_;
  VkDeviceMemory colorMemory_;

  VkFormat depthFormat_;
  VkImage depth_;
  VkImageView depthView_;
  VkDeviceMemory depthMemory_;

  VkRenderPass renderpass_;
  VkFramebuffer framebuffer_;

  VkPipeline pipeline_;
  VkPipelineCache pipelineCache_;
  VkPipelineLayout pipelineLayout_;
  VkShaderModule shaderVertex_;
  VkShaderModule shaderFragment_;

  uint32_t getMemoryTypeIndex(uint32_t typeMask, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties deviceMemProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &deviceMemProps);
    for (int i = 0; i < deviceMemProps.memoryTypeCount; ++i) {
      if ((typeMask & 1) == 1) {
        if ((deviceMemProps.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
        }
      }
      typeMask >>= 1;
    }
    throw std::runtime_error("can not find suitable memory type");
  }

  VkFormat getSupportedDepthFormat() {
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };
    for (auto format : depthFormats) {
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &formatProps);
      if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        return format;
      }
    }
    throw std::runtime_error("can not find supported depth format");
  }

  VkShaderModule loadShader(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
    if (file.is_open()) {
      size_t size = file.tellg();
      assert(size > 0);
      file.seekg(0, std::ios::beg);
      char *shaderCode = new char[size];
      file.read(shaderCode, size);
      file.close();

      VkShaderModule shader;
      VkShaderModuleCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = size;
      createInfo.pCode = (uint32_t *) shaderCode;

      vkCreateShaderModule(device_, &createInfo, nullptr, &shader);

      delete[] shaderCode;
      return shader;
    }
    throw std::runtime_error("shader file dost not exist");
  }

  // create buffer and associated memory, copy data to memory if data != nullptr
  void createBuffer(void *pData, VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer *pBuffer, VkDeviceMemory *pMemory) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = bufferUsageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    CHECK_VK_SUCCESS(vkCreateBuffer(device_, &bufferCreateInfo, nullptr, pBuffer));

    VkMemoryRequirements vertexMemReqs{};
    vkGetBufferMemoryRequirements(device_, *pBuffer, &vertexMemReqs);
    VkMemoryAllocateInfo vertexMemAllocateInfo{};
    vertexMemAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexMemAllocateInfo.allocationSize = vertexMemReqs.size;
    vertexMemAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(vertexMemReqs.memoryTypeBits, memoryPropertyFlags);
    CHECK_VK_SUCCESS(vkAllocateMemory(device_, &vertexMemAllocateInfo, nullptr, pMemory));

    if (pData != nullptr) {
      void *mapped;
      CHECK_VK_SUCCESS(vkMapMemory(device_, *pMemory, 0, size, 0, &mapped));
      memcpy(mapped, pData, size);
      vkUnmapMemory(device_, *pMemory);
    }

    CHECK_VK_SUCCESS(vkBindBufferMemory(device_, *pBuffer, *pMemory, 0));
  }

  void create2DImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryProperties,
                     VkImage *pImage, VkDeviceMemory *pMemory, VkImageView *pImageView) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.format = imageFormat;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = tiling;
    imageInfo.usage = imageUsageFlags;
    CHECK_VK_SUCCESS(vkCreateImage(device_, &imageInfo, nullptr, pImage));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device_, *pImage, &memReqs);
    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, memoryProperties);
    CHECK_VK_SUCCESS(vkAllocateMemory(device_, &memAllocInfo, nullptr, pMemory));
    CHECK_VK_SUCCESS(vkBindImageMemory(device_, *pImage, *pMemory, 0));

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = *pImage;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = imageFormat;
    imageViewInfo.subresourceRange = {};
    VkImageAspectFlags aspectMask{};
    switch (imageFormat) {
      case VK_FORMAT_D16_UNORM:aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case VK_FORMAT_D16_UNORM_S8_UINT:aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      case VK_FORMAT_D24_UNORM_S8_UINT:aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      case VK_FORMAT_D32_SFLOAT:aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case VK_FORMAT_D32_SFLOAT_S8_UINT:aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      default:aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    imageViewInfo.subresourceRange.aspectMask = aspectMask;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    CHECK_VK_SUCCESS(vkCreateImageView(device_, &imageViewInfo, nullptr, pImageView));
  }

  HeadlessRenderer() {
    // create instance
    {
      VkApplicationInfo appInfo{};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.apiVersion = VK_API_VERSION_1_0;
      appInfo.pApplicationName = "HeadlessRenderer";
      appInfo.pEngineName = "HeadlessRenderer";

      VkInstanceCreateInfo instanceInfo{};
      instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instanceInfo.pApplicationInfo = &appInfo;
//      const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
//      instanceInfo.enabledLayerCount = 1;
//      instanceInfo.ppEnabledLayerNames = &validationLayerName;
      CHECK_VK_SUCCESS(vkCreateInstance(&instanceInfo, nullptr, &instance_));
    }

    // select physical device
    {
      uint32_t deviceCount;
      vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
      physicalDevice_ = devices[0];
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(physicalDevice_, &props);
      std::cout << "select " << props.deviceName << "\n";
    }

    // find a suitable queue family index
    {
      uint32_t familyCount;
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);
      std::vector<VkQueueFamilyProperties> props(familyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, props.data());
      for (int i = 0; i < props.size(); ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
          queueFamilyIndex_ = i;
          break;
        }
      }
      if (queueFamilyIndex_ == -1) {
        throw std::runtime_error("can not find a suitable queue family");
      }
    }

    // create logical device
    {
      float queuePriority = 1.0f;
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamilyIndex_;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      VkDeviceCreateInfo deviceCreateInfo{};
      deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCreateInfo.queueCreateInfoCount = 1;
      deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
      CHECK_VK_SUCCESS(vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_));
      vkGetDeviceQueue(device_, queueFamilyIndex_, 0, &queue_);
    }

    // create command pool
    {
      VkCommandPoolCreateInfo poolCreateInfo{};
      poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolCreateInfo.queueFamilyIndex = queueFamilyIndex_;
      poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      CHECK_VK_SUCCESS(vkCreateCommandPool(device_, &poolCreateInfo, nullptr, &commandPool_));
    }

    struct Vertex {
      float position[3];
      float color[3];
    };
    std::vector<Vertex> vertices = {
        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };
    std::vector<uint32_t> indices = {0, 1, 2};
    // load mesh
    {
      Assimp::Importer importer;
      const aiScene* scene = importer.ReadFile("/home/shq/XYZPoseLab/brake_disk/models/model.ply", aiProcess_Triangulate);
      if (scene) {
        if (scene->mNumMeshes == 1) {
          const aiMesh* mesh = scene->mMeshes[0];
          if (mesh->mNumFaces > 0 && mesh->mNumVertices > 0) {
            vertices.clear();
            indices.clear();
            for (int i = 0; i < mesh->mNumVertices; ++i) {
              Vertex vertex;
              vertex.position[0] = mesh->mVertices[i].x;
              vertex.position[1] = mesh->mVertices[i].y;
              vertex.position[2] = mesh->mVertices[i].z;
              vertex.color[0] = 1.0f;
              vertex.color[1] = 1.0f;
              vertex.color[2] = 1.0f;
              vertices.push_back(vertex);
            }
            for (int i = 0; i < mesh->mNumFaces; ++i) {
              indices.push_back(mesh->mFaces[i].mIndices[0]);
              indices.push_back(mesh->mFaces[i].mIndices[1]);
              indices.push_back(mesh->mFaces[i].mIndices[2]);
            }
          }
        }
      }
    }
    printf("#vertices = %lu\n", vertices.size());
    printf("#indices = %lu\n", indices.size());

    // copy vertex data to device local buffer
    {
      auto t1 = std::chrono::high_resolution_clock::now();
      const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
      const VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

      createBuffer(vertices.data(),
                   vertexBufferSize,
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   &vertexBuffer_,
                   &vertexMemory_);

      createBuffer(indices.data(),
                   indexBufferSize,
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   &indexBuffer_,
                   &indexMemory_);
      auto t2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
      std::cout << "copy vertex/index buffer to device cost " << duration << " us\n";
    }

    // create image attachments
    uint32_t width = 2048;
    uint32_t height = 1536;
    {
      colorFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
      create2DImage(width,
                    height,
                    colorFormat_,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &color_,
                    &colorMemory_,
                    &colorView_);

      depthFormat_ = getSupportedDepthFormat();
      create2DImage(width,
                    height,
                    depthFormat_,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &depth_,
                    &depthMemory_,
                    &depthView_);
    }

    // create render pass
    {
      std::array<VkAttachmentDescription, 2> attachmentDescriptions;
      attachmentDescriptions[0].format = colorFormat_;
      attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

      attachmentDescriptions[1].format = depthFormat_;
      attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

      VkAttachmentReference colorRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
      VkAttachmentReference depthRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

      VkSubpassDescription subpassDescription{};
      subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescription.colorAttachmentCount = 1;
      subpassDescription.pColorAttachments = &colorRef;
      subpassDescription.pDepthStencilAttachment = &depthRef;

      // ???
      std::array<VkSubpassDependency, 2> subpassDependencys;
      subpassDependencys[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      subpassDependencys[0].dstSubpass = 0;
      subpassDependencys[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      subpassDependencys[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      subpassDependencys[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      subpassDependencys[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      subpassDependencys[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      subpassDependencys[1].srcSubpass = 0;
      subpassDependencys[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      subpassDependencys[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      subpassDependencys[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      subpassDependencys[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      subpassDependencys[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      subpassDependencys[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      VkRenderPassCreateInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = attachmentDescriptions.size();
      renderPassInfo.pAttachments = attachmentDescriptions.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpassDescription;
      renderPassInfo.dependencyCount = subpassDependencys.size();
      renderPassInfo.pDependencies = subpassDependencys.data();
      CHECK_VK_SUCCESS(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderpass_));
    }

    // create framebuffer
    {
      std::array<VkImageView, 2> attachments;
      attachments[0] = colorView_;
      attachments[1] = depthView_;

      VkFramebufferCreateInfo bufferInfo{};
      bufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      bufferInfo.renderPass = renderpass_;
      bufferInfo.attachmentCount = attachments.size();
      bufferInfo.pAttachments = attachments.data();
      bufferInfo.width = width;
      bufferInfo.height = height;
      bufferInfo.layers = 1;
      CHECK_VK_SUCCESS(vkCreateFramebuffer(device_, &bufferInfo, nullptr, &framebuffer_));
    }

    // create graphics pipeline
    struct MeshPushConstants {
      glm::mat4 model_view;
      glm::mat4 proj;
      float far_z;
    };
    {
      VkPipelineCacheCreateInfo cacheInfo{};
      cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
      CHECK_VK_SUCCESS(vkCreatePipelineCache(device_, &cacheInfo, nullptr, &pipelineCache_));

      VkGraphicsPipelineCreateInfo pipeInfo{};
      pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipeInfo.pNext = nullptr;
      pipeInfo.renderPass = renderpass_;
      pipeInfo.subpass = 0;
      pipeInfo.flags = 0;

      std::vector<VkPushConstantRange> pushConstants = {
          initializePushConstanceRange(sizeof(MeshPushConstants), 0, VK_SHADER_STAGE_VERTEX_BIT),
      };
      VkPipelineLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      layoutInfo.pNext = nullptr;
      layoutInfo.pushConstantRangeCount = pushConstants.size();
      layoutInfo.pPushConstantRanges = pushConstants.data();
      layoutInfo.setLayoutCount = 0;
      layoutInfo.pSetLayouts = nullptr;
      CHECK_VK_SUCCESS(vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &pipelineLayout_));
      pipeInfo.layout = pipelineLayout_;

      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
      shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      shaderStages[0].module = loadShader(VK_EXAMPLE_DATA_DIR "shaders/glsl/myrenderheadless/triangle.vert.spv");
      shaderStages[0].pName = "main";
      shaderStages[0].pSpecializationInfo = nullptr;
      shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      shaderStages[1].module = loadShader(VK_EXAMPLE_DATA_DIR "shaders/glsl/myrenderheadless/triangle.frag.spv");
      shaderStages[1].pName = "main";
      shaderStages[1].pSpecializationInfo = nullptr;
      shaderVertex_ = shaderStages[0].module;
      shaderFragment_ = shaderStages[1].module;
      pipeInfo.stageCount = shaderStages.size();
      pipeInfo.pStages = shaderStages.data();

      VkVertexInputBindingDescription vertexBinding = initializeVertexInputBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
      std::array<VkVertexInputAttributeDescription, 2> vertexAttributes{
          initializeVertexInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
          initializeVertexInputAttribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)
      };
      VkPipelineVertexInputStateCreateInfo vertexInputState{};
      vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      vertexInputState.vertexBindingDescriptionCount = 1;
      vertexInputState.pVertexBindingDescriptions = &vertexBinding;
      vertexInputState.vertexAttributeDescriptionCount = vertexAttributes.size();
      vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();
      pipeInfo.pVertexInputState = &vertexInputState;

      VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
      inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      inputAssemblyState.primitiveRestartEnable = VK_FALSE;
      pipeInfo.pInputAssemblyState = &inputAssemblyState;

      VkPipelineViewportStateCreateInfo viewportState{};
      viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      viewportState.viewportCount = 1;
      viewportState.scissorCount = 1;
      pipeInfo.pViewportState = &viewportState;

      VkPipelineRasterizationStateCreateInfo rasterState{};
      rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rasterState.depthClampEnable = VK_FALSE;
      rasterState.rasterizerDiscardEnable = VK_FALSE;
      rasterState.polygonMode = VK_POLYGON_MODE_FILL;
      rasterState.cullMode = VK_CULL_MODE_NONE;
      rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      rasterState.lineWidth = 1.0f;
      rasterState.depthBiasEnable = VK_FALSE;
      pipeInfo.pRasterizationState = &rasterState;

      VkPipelineMultisampleStateCreateInfo multisampleState{};
      multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      pipeInfo.pMultisampleState = &multisampleState;

      VkPipelineDepthStencilStateCreateInfo depthStencilState{};
      depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depthStencilState.depthTestEnable = VK_TRUE;
      depthStencilState.depthWriteEnable = VK_TRUE;
      depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
      pipeInfo.pDepthStencilState = &depthStencilState;

      VkPipelineColorBlendAttachmentState attachmentState{};
      attachmentState.blendEnable = VK_FALSE;
      attachmentState.colorWriteMask = 0xf;
      VkPipelineColorBlendStateCreateInfo colorBlendState{};
      colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      colorBlendState.attachmentCount = 1;
      colorBlendState.pAttachments = &attachmentState;
      pipeInfo.pColorBlendState = &colorBlendState;

      std::vector<VkDynamicState> dynamicStates {
          VK_DYNAMIC_STATE_VIEWPORT,
          VK_DYNAMIC_STATE_SCISSOR
      };
      VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
      dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      dynamicStateInfo.dynamicStateCount = dynamicStates.size();
      dynamicStateInfo.pDynamicStates = dynamicStates.data();
      pipeInfo.pDynamicState = &dynamicStateInfo;

      pipeInfo.basePipelineIndex = -1;
      pipeInfo.basePipelineHandle = VK_NULL_HANDLE;


      CHECK_VK_SUCCESS(vkCreateGraphicsPipelines(device_, pipelineCache_, 1, &pipeInfo, nullptr, &pipeline_));
    }

    // Create command buffer
    {
      auto t1 = std::chrono::high_resolution_clock::now();
      VkCommandBuffer cmdBuffer;
      VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
      cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmdBufferAllocateInfo.commandBufferCount = 1;
      cmdBufferAllocateInfo.commandPool = commandPool_;
      cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      CHECK_VK_SUCCESS(vkAllocateCommandBuffers(device_, &cmdBufferAllocateInfo, &cmdBuffer));

      VkCommandBufferBeginInfo cmdBufferBeginInfo{};
      cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      CHECK_VK_SUCCESS(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

      VkClearValue clearValues[2];
      clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
      clearValues[1].depthStencil = {1.0f, 0};
      VkRenderPassBeginInfo renderPassBegin{};
      renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassBegin.renderPass = renderpass_;
      renderPassBegin.framebuffer = framebuffer_;
      renderPassBegin.renderArea.extent.width = width;
      renderPassBegin.renderArea.extent.height = height;
      renderPassBegin.clearValueCount = 2;
      renderPassBegin.pClearValues = clearValues;
      vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport;
      viewport.width = (float)width;
      viewport.height = (float)height;
      viewport.minDepth = (float)0.0f;
      viewport.maxDepth = (float)1.0f;
      vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

      VkRect2D scissor{};
      scissor.extent.width = width;
      scissor.extent.height = height;
      vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

      VkDeviceSize offsets[1] = {0};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer_, offsets);

      vkCmdBindIndexBuffer(cmdBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

      std::vector<glm::vec3> pos = {
          glm::vec3(-0.6f, 0.0f, 0.0f),
          glm::vec3(-0.3f, 0.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.3f, 0.0f, 0.0f),
//          glm::vec3(0.6f, 0.0f, 0.0f),
          glm::vec3(-0.6f, 0.3f, 0.0f),
          glm::vec3(-0.3f, 0.3f, 0.0f),
          glm::vec3(0.0f, 0.3f, 0.0f),
          glm::vec3(0.3f, 0.3f, 0.0f),
//          glm::vec3(0.6f, 0.3f, 0.0f),
          glm::vec3(-0.6f, -0.3f, 0.0f),
          glm::vec3(-0.3f, -0.3f, 0.0f),
          glm::vec3(0.0f, -0.3f, 0.0f),
          glm::vec3(0.3f, -0.3f, 0.0f),
//          glm::vec3(0.6f, -0.3f, 0.0f),
      };

      glm::mat4 view = glm::mat4(1);
      view[1][1] = -1;
      view[2][2] = -1;
      view[3][2] = 2;

      glm::mat4 K = glm::mat4(1);
      K[0][0] = 2413;
      K[1][1] = 2413;
      K[3][0] = 2048 / 2;
      K[3][1] = 1536 / 2;

      glm::mat4 img2ndc = glm::mat4(1);
      img2ndc[0][0] = 2.0f / 2048.0f;
      img2ndc[1][1] = 2.0f / 1536.0f;
      img2ndc[3][0] = -1.0f;
      img2ndc[3][1] = -1.0f;

      MeshPushConstants constants;
      constants.proj = img2ndc * K;
      constants.far_z = 4.0f;

      for (auto v: pos) {
        constants.model_view = glm::inverse(view) * glm::translate(glm::mat4(1.0f), v);
        vkCmdPushConstants(cmdBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
        vkCmdDrawIndexed(cmdBuffer, indices.size(), 1, 0, 0, 0);
      }

      vkCmdEndRenderPass(cmdBuffer);

      CHECK_VK_SUCCESS(vkEndCommandBuffer(cmdBuffer));

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &cmdBuffer;
      vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
      vkDeviceWaitIdle(device_);

      auto t2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
      std::cout << "render cost " << duration << " us\n";
    }

    // copy image to host
    VkImage dstImage;
    VkDeviceMemory dstImageMemory;
    VkImageView dstImageView;
    {
      create2DImage(width, height,
                    colorFormat_,
                    VK_IMAGE_TILING_LINEAR,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &dstImage,
                    &dstImageMemory,
                    &dstImageView);

      VkCommandBuffer cmdBuffer;
      VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
      cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmdBufferAllocateInfo.commandBufferCount = 1;
      cmdBufferAllocateInfo.commandPool = commandPool_;
      cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      CHECK_VK_SUCCESS(vkAllocateCommandBuffers(device_, &cmdBufferAllocateInfo, &cmdBuffer));

      VkCommandBufferBeginInfo cmdBufferBeginInfo{};
      cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      CHECK_VK_SUCCESS(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

      // destination image layout to TRANSFER_DST_OPTIMAL
      VkImageMemoryBarrier imageBarrier{};
      imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageBarrier.srcAccessMask = 0;
      imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.image = dstImage;
      imageBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCmdPipelineBarrier(cmdBuffer,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0,
                           0, nullptr,
                           0, nullptr,
                           1, &imageBarrier);

      // source image layout is already TRANSFER_SRC_OPTIMAL

      VkImageCopy copyRegion{};
      copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copyRegion.srcSubresource.layerCount = 1;
      copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copyRegion.dstSubresource.layerCount = 1;
      copyRegion.extent.width = width;
      copyRegion.extent.height = height;
      copyRegion.extent.depth = 1;
      vkCmdCopyImage(cmdBuffer, color_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &copyRegion);

      // destination image to general layout
      imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageBarrier.image = dstImage;
      imageBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCmdPipelineBarrier(cmdBuffer,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0,
                           0, nullptr,
                           0, nullptr,
                           1, &imageBarrier);


      CHECK_VK_SUCCESS(vkEndCommandBuffer(cmdBuffer));

      auto t1 = std::chrono::high_resolution_clock::now();

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &cmdBuffer;
      vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
      vkDeviceWaitIdle(device_);

      auto t2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
      std::cout << "copy to host cost " << duration << " us\n";
    }

    // save image
    const char * imageData;
    {
      VkImageSubresource subResource{};
      subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      VkSubresourceLayout subResourceLayout{};
      vkGetImageSubresourceLayout(device_, dstImage, &subResource, &subResourceLayout);

      vkMapMemory(device_, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&imageData);
      imageData += subResourceLayout.offset;

      const char *filename = "myheadless.ppm";
      std::ofstream file(filename, std::ios::out | std::ios::binary);

      // ppm header
      file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";
      for (int32_t y = 0; y < height; y++) {
        auto *row = (unsigned int *) imageData;
        for (int32_t x = 0; x < width; x++) {
          file.write((char *) row, 3);
          row++;
        }
        imageData += subResourceLayout.rowPitch;
      }
      file.close();

    }
  }

  ~HeadlessRenderer() {
    vkDestroyPipeline(device_, pipeline_, nullptr);
    vkDestroyPipelineCache(device_, pipelineCache_, nullptr);
    vkDestroyShaderModule(device_, shaderVertex_, nullptr);
    vkDestroyShaderModule(device_, shaderFragment_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    vkDestroyRenderPass(device_, renderpass_, nullptr);
    vkFreeMemory(device_, vertexMemory_, nullptr);
    vkFreeMemory(device_, indexMemory_, nullptr);
    vkFreeMemory(device_, colorMemory_, nullptr);
    vkFreeMemory(device_, depthMemory_, nullptr);
    vkDestroyImageView(device_, colorView_, nullptr);
    vkDestroyImageView(device_, depthView_, nullptr);
    vkDestroyImage(device_, color_, nullptr);
    vkDestroyImage(device_, depth_, nullptr);
    vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    vkDestroyBuffer(device_, indexBuffer_, nullptr);
    vkDestroyCommandPool(device_, commandPool_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroyInstance(instance_, nullptr);
  }
};

int main(int argc, char **argv) {
  HeadlessRenderer renderer;
  return 0;
}
