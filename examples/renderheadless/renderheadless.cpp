#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"

#define LOG(...) printf(__VA_ARGS__)

class VulkanExample {
 public:
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  uint32_t queueFamilyIndex;
  VkPipelineCache pipelineCache;
  VkQueue queue;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  std::vector<VkShaderModule> shaderModules;
  VkBuffer vertexBuffer, indexBuffer;
  VkDeviceMemory vertexMemory, indexMemory;

  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
  };
  int32_t width, height;
  VkFramebuffer framebuffer;
  FrameBufferAttachment colorAttachment, depthAttachment;
  VkRenderPass renderPass;

  VkDebugReportCallbackEXT debugReportCallback{};

  uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
      if ((typeBits & 1) == 1) {
        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
        }
      }
      typeBits >>= 1;
    }
    return 0;
  }

  VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer *buffer, VkDeviceMemory *memory, VkDeviceSize size, void *data = nullptr) const {
    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, memoryPropertyFlags);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory));

    if (data != nullptr) {
      void *mapped;
      VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped));
      memcpy(mapped, data, size);
      vkUnmapMemory(device, *memory);
    }

    VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0));

    return VK_SUCCESS;
  }

  /*
      Submit command buffer to a queue and wait for fence until queue operations have been finished
  */
  void submitWork(VkCommandBuffer cmdBuffer, VkQueue targetQueue) const {
    VkSubmitInfo submitInfo = vks::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo();
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    VK_CHECK_RESULT(vkQueueSubmit(targetQueue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device, fence, nullptr);
  }

  VulkanExample() {
    LOG("Running headless rendering example\n");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan headless example";
    appInfo.pEngineName = "VulkanExample";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    /*
        Vulkan instance creation (without surface extensions)
    */
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

    /*
        Vulkan device creation
    */
    uint32_t deviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));
    if (!physicalDevices.empty())
      physicalDevice = physicalDevices[0];
    else
      throw std::runtime_error("no physical device supported");

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    LOG("GPU: %s\n", deviceProperties.deviceName);

    // Request a single graphics queue
    const float defaultQueuePriority(0.0f);
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
      if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queueFamilyIndex = i;
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = i;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
        break;
      }
    }
    // Create logical device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    // Get a graphics queue
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // Command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));

    /*
        Prepare vertex and index buffers
    */
    struct Vertex {
      float position[3];
      float color[3];
    };
    {
      std::vector<Vertex> vertices = {
          {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
          {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
          {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}
      };
      std::vector<uint32_t> indices = {0, 1, 2};

      const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
      const VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingMemory;

      // Command buffer for copy commands (reused)
      VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
      VkCommandBuffer copyCmd;
      VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));
      VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

      // Copy input data to VRAM using a staging buffer
      {
        auto t1 = std::chrono::high_resolution_clock::now();
        // Vertices
        createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            &stagingMemory,
            vertexBufferSize,
            vertices.data());

        createBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &vertexBuffer,
            &vertexMemory,
            vertexBufferSize);

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
        VkBufferCopy copyRegion = {};
        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, stagingBuffer, vertexBuffer, 1, &copyRegion);
        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        submitWork(copyCmd, queue);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        // Indices
        createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            &stagingMemory,
            indexBufferSize,
            indices.data());

        createBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &indexBuffer,
            &indexMemory,
            indexBufferSize);

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, stagingBuffer, indexBuffer, 1, &copyRegion);
        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        submitWork(copyCmd, queue);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
        std::cout << "copy vertex buffer cost " << duration.count() << " us\n";
      }
    }

    /*
        Create framebuffer attachments
    */
    width = 2048;
    height = 1536;
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat;
    vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
    {
      // Color attachment
      VkImageCreateInfo image = vks::initializers::imageCreateInfo();
      image.imageType = VK_IMAGE_TYPE_2D;
      image.format = colorFormat;
      image.extent.width = width;
      image.extent.height = height;
      image.extent.depth = 1;
      image.mipLevels = 1;
      image.arrayLayers = 1;
      image.samples = VK_SAMPLE_COUNT_1_BIT;
      image.tiling = VK_IMAGE_TILING_OPTIMAL;
      image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

      VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
      VkMemoryRequirements memReqs;

      VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &colorAttachment.image));
      vkGetImageMemoryRequirements(device, colorAttachment.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &colorAttachment.memory));
      VK_CHECK_RESULT(vkBindImageMemory(device, colorAttachment.image, colorAttachment.memory, 0));

      VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
      colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      colorImageView.format = colorFormat;
      colorImageView.subresourceRange = {};
      colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      colorImageView.subresourceRange.baseMipLevel = 0;
      colorImageView.subresourceRange.levelCount = 1;
      colorImageView.subresourceRange.baseArrayLayer = 0;
      colorImageView.subresourceRange.layerCount = 1;
      colorImageView.image = colorAttachment.image;
      VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &colorAttachment.view));

      // Depth stencil attachment
      image.format = depthFormat;
      image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

      VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthAttachment.image));
      vkGetImageMemoryRequirements(device, depthAttachment.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthAttachment.memory));
      VK_CHECK_RESULT(vkBindImageMemory(device, depthAttachment.image, depthAttachment.memory, 0));

      VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
      depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      depthStencilView.format = depthFormat;
      depthStencilView.flags = 0;
      depthStencilView.subresourceRange = {};
      depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      depthStencilView.subresourceRange.baseMipLevel = 0;
      depthStencilView.subresourceRange.levelCount = 1;
      depthStencilView.subresourceRange.baseArrayLayer = 0;
      depthStencilView.subresourceRange.layerCount = 1;
      depthStencilView.image = depthAttachment.image;
      VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthAttachment.view));
    }

    /*
        Create renderpass
    */
    {
      std::array<VkAttachmentDescription, 2> attachmentDescription = {};
      // Color attachment
      attachmentDescription[0].format = colorFormat;
      attachmentDescription[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescription[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescription[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescription[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescription[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescription[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      // Depth attachment
      attachmentDescription[1].format = depthFormat;
      attachmentDescription[1].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescription[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescription[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescription[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescription[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
      VkAttachmentReference depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

      VkSubpassDescription subpassDescription = {};
      subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescription.colorAttachmentCount = 1;
      subpassDescription.pColorAttachments = &colorReference;
      subpassDescription.pDepthStencilAttachment = &depthReference;

      // Use subpass dependencies for layout transitions
      std::array<VkSubpassDependency, 2> dependencies{};

      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      // Create the actual renderpass
      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescription.size());
      renderPassInfo.pAttachments = attachmentDescription.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpassDescription;
      renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
      renderPassInfo.pDependencies = dependencies.data();
      VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

      VkImageView attachments[2];
      attachments[0] = colorAttachment.view;
      attachments[1] = depthAttachment.view;

      VkFramebufferCreateInfo framebufferCreateInfo = vks::initializers::framebufferCreateInfo();
      framebufferCreateInfo.renderPass = renderPass;
      framebufferCreateInfo.attachmentCount = 2;
      framebufferCreateInfo.pAttachments = attachments;
      framebufferCreateInfo.width = width;
      framebufferCreateInfo.height = height;
      framebufferCreateInfo.layers = 1;
      VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer));
    }

    /*
        Prepare graphics pipeline
    */
    {
      std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
      VkDescriptorSetLayoutCreateInfo descriptorLayout =
          vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
      VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

      VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
          vks::initializers::pipelineLayoutCreateInfo(nullptr, 0);

      // MVP via push constant block
      VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);
      pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
      pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

      VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

      VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
      pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
      VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

      // Create pipeline
      VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
          vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

      VkPipelineRasterizationStateCreateInfo rasterizationState =
          vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

      VkPipelineColorBlendAttachmentState blendAttachmentState =
          vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

      VkPipelineColorBlendStateCreateInfo colorBlendState =
          vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

      VkPipelineDepthStencilStateCreateInfo depthStencilState =
          vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

      VkPipelineViewportStateCreateInfo viewportState =
          vks::initializers::pipelineViewportStateCreateInfo(1, 1);

      VkPipelineMultisampleStateCreateInfo multisampleState =
          vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

      std::vector<VkDynamicState> dynamicStateEnables = {
          VK_DYNAMIC_STATE_VIEWPORT,
          VK_DYNAMIC_STATE_SCISSOR
      };
      VkPipelineDynamicStateCreateInfo dynamicState =
          vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

      VkGraphicsPipelineCreateInfo pipelineCreateInfo =
          vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);

      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

      pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
      pipelineCreateInfo.pRasterizationState = &rasterizationState;
      pipelineCreateInfo.pColorBlendState = &colorBlendState;
      pipelineCreateInfo.pMultisampleState = &multisampleState;
      pipelineCreateInfo.pViewportState = &viewportState;
      pipelineCreateInfo.pDepthStencilState = &depthStencilState;
      pipelineCreateInfo.pDynamicState = &dynamicState;
      pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
      pipelineCreateInfo.pStages = shaderStages.data();

      // Vertex bindings an attributes
      // Binding description
      std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
          vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
      };

      // Attribute descriptions
      std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
          vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                    // Position
          vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),    // Color
      };

      VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
      vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
      vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
      vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
      vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

      pipelineCreateInfo.pVertexInputState = &vertexInputState;

      // TODO: There is no command line arguments parsing (nor Android settings) for this
      // example, so we have no way of picking between GLSL or HLSL shaders.
      // Hard-code to glsl for now.
      const std::string shadersPath = getAssetPath() + "shaders/glsl/renderheadless/";

      shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      shaderStages[0].pName = "main";
      shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      shaderStages[1].pName = "main";
      shaderStages[0].module = vks::tools::loadShader((shadersPath + "triangle.vert.spv").c_str(), device);
      shaderStages[1].module = vks::tools::loadShader((shadersPath + "triangle.frag.spv").c_str(), device);
      shaderModules = {shaderStages[0].module, shaderStages[1].module};
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
    }

    /*
        Command buffer creation
    */
    {
      VkCommandBuffer cmdBuffer;
      VkCommandBufferAllocateInfo cmdBufAllocateInfo =
          vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
      VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

      VkCommandBufferBeginInfo cmdBufInfo =
          vks::initializers::commandBufferBeginInfo();

      VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

      VkClearValue clearValues[2];
      clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
      clearValues[1].depthStencil = {1.0f, 0};

      VkRenderPassBeginInfo renderPassBeginInfo = {};
      renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassBeginInfo.renderArea.extent.width = width;
      renderPassBeginInfo.renderArea.extent.height = height;
      renderPassBeginInfo.clearValueCount = 2;
      renderPassBeginInfo.pClearValues = clearValues;
      renderPassBeginInfo.renderPass = renderPass;
      renderPassBeginInfo.framebuffer = framebuffer;

      vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = {};
      viewport.height = (float) height;
      viewport.width = (float) width;
      viewport.minDepth = (float) 0.0f;
      viewport.maxDepth = (float) 1.0f;
      vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

      // Update dynamic scissor state
      VkRect2D scissor = {};
      scissor.extent.width = width;
      scissor.extent.height = height;
      vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

      // Render scene
      VkDeviceSize offsets[1] = {0};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
      vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

      std::vector<glm::vec3> pos = {
          glm::vec3(-1.5f, 0.0f, -4.0f),
          glm::vec3(0.0f, 0.0f, -2.5f),
          glm::vec3(1.5f, 0.0f, -4.0f),
      };

      for (auto v: pos) {
        glm::mat4 mvpMatrix = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.1f, 256.0f) * glm::translate(glm::mat4(1.0f), v);
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvpMatrix), &mvpMatrix);
        vkCmdDrawIndexed(cmdBuffer, 3, 1, 0, 0, 0);
      }

      vkCmdEndRenderPass(cmdBuffer);

      VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

      auto t1 = std::chrono::high_resolution_clock::now();
      submitWork(cmdBuffer, queue);

      vkDeviceWaitIdle(device);
      auto t2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
      std::cout << "render cost " << duration.count() << " us\n";
    }

    /*
        Copy framebuffer image to host visible image
    */
    const char *imagedata;
    {
      // Create the linear tiled destination image to copy to and to read the memory from
      VkImageCreateInfo imgCreateInfo(vks::initializers::imageCreateInfo());
      imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
      imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
      imgCreateInfo.extent.width = width;
      imgCreateInfo.extent.height = height;
      imgCreateInfo.extent.depth = 1;
      imgCreateInfo.arrayLayers = 1;
      imgCreateInfo.mipLevels = 1;
      imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
      imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      // Create the image
      VkImage dstImage;
      VK_CHECK_RESULT(vkCreateImage(device, &imgCreateInfo, nullptr, &dstImage));
      // Create memory to back up the image
      VkMemoryRequirements memRequirements;
      VkMemoryAllocateInfo memAllocInfo(vks::initializers::memoryAllocateInfo());
      VkDeviceMemory dstImageMemory;
      vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
      memAllocInfo.allocationSize = memRequirements.size;
      // Memory must be host visible to copy from
      memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImageMemory));
      VK_CHECK_RESULT(vkBindImageMemory(device, dstImage, dstImageMemory, 0));

      // Do the actual blit from the offscreen image to our host visible destination image
      VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
      VkCommandBuffer copyCmd;
      VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));
      VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
      VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

      // Transition destination image to transfer destination layout
      vks::tools::insertImageMemoryBarrier(
          copyCmd,
          dstImage,
          0,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

      // colorAttachment.image is already in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be transitioned

      VkImageCopy imageCopyRegion{};
      imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.srcSubresource.layerCount = 1;
      imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.dstSubresource.layerCount = 1;
      imageCopyRegion.extent.width = width;
      imageCopyRegion.extent.height = height;
      imageCopyRegion.extent.depth = 1;

      vkCmdCopyImage(
          copyCmd,
          colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          1,
          &imageCopyRegion);

      // Transition destination image to general layout, which is the required layout for mapping the image memory later on
      vks::tools::insertImageMemoryBarrier(
          copyCmd,
          dstImage,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          VK_ACCESS_MEMORY_READ_BIT,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_IMAGE_LAYOUT_GENERAL,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

      VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

      auto t1 = std::chrono::high_resolution_clock::now();
      submitWork(copyCmd, queue);

      // Get layout of the image (including row pitch)
      VkImageSubresource subResource{};
      subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      VkSubresourceLayout subResourceLayout;

      vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

      // Map image memory so we can start copying from it
      vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void **) &imagedata);
      imagedata += subResourceLayout.offset;
      auto t2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
      std::cout << "transfer image cost " << duration.count() << " us\n";

      /*
          Save host visible framebuffer image to disk (ppm format)
      */

      const char *filename = "headless.ppm";
      std::ofstream file(filename, std::ios::out | std::ios::binary);

      // ppm header
      file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";

      // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
      // Check if source is BGR and needs swizzle
      std::vector<VkFormat> formatsBGR = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM};
      const bool colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());

      // ppm binary pixel data
      for (int32_t y = 0; y < height; y++) {
        auto *row = (unsigned int *) imagedata;
        for (int32_t x = 0; x < width; x++) {
          if (colorSwizzle) {
            file.write((char *) row + 2, 1);
            file.write((char *) row + 1, 1);
            file.write((char *) row, 1);
          } else {
            file.write((char *) row, 3);
          }
          row++;
        }
        imagedata += subResourceLayout.rowPitch;
      }
      file.close();

      LOG("Framebuffer image saved to %s\n", filename);

      // Clean up resources
      vkUnmapMemory(device, dstImageMemory);
      vkFreeMemory(device, dstImageMemory, nullptr);
      vkDestroyImage(device, dstImage, nullptr);
    }

    vkQueueWaitIdle(queue);
  }

  ~VulkanExample() {
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexMemory, nullptr);
    vkDestroyImageView(device, colorAttachment.view, nullptr);
    vkDestroyImage(device, colorAttachment.image, nullptr);
    vkFreeMemory(device, colorAttachment.memory, nullptr);
    vkDestroyImageView(device, depthAttachment.view, nullptr);
    vkDestroyImage(device, depthAttachment.image, nullptr);
    vkFreeMemory(device, depthAttachment.memory, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto shadermodule: shaderModules) {
      vkDestroyShaderModule(device, shadermodule, nullptr);
    }
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
  }
};

int main() {
  auto *vulkanExample = new VulkanExample();
  delete (vulkanExample);
  return 0;
}