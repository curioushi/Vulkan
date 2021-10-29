#include <cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <string>
#include <vulkan/vulkan.h>

#define CHECK_VK_SUCCESS(ret) \
  if ((ret) != VK_SUCCESS) {  \
    std::cerr << "check vk success failed at line: " << __LINE__; \
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

  VkShaderModule loadShader(const std::string& filename) {
    std::ifstream file(filename, std::ios_base::binary | std::ios_base::in | std::ios_base::ate);
    if (file.is_open()) {
      size_t size = file.tellg();
      assert(size > 0);
      file.seekg(0, std::ios_base::beg);
      char* shaderCode = new char[size];
      file.read(shaderCode, size);
      file.close();

      VkShaderModule shader;
      VkShaderModuleCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = size;
      createInfo.pCode = (uint32_t*)shaderCode;

      vkCreateShaderModule(device_, &createInfo, nullptr, &shader);

      delete [] shaderCode;
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

  void create2DImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryProperties,
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
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
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
      case VK_FORMAT_D16_UNORM:
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case VK_FORMAT_D16_UNORM_S8_UINT:
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      case VK_FORMAT_D24_UNORM_S8_UINT:
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      case VK_FORMAT_D32_SFLOAT:
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case VK_FORMAT_D32_SFLOAT_S8_UINT:
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      default:
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamilyIndex_;
      queueCreateInfo.queueCount = 1;
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

    // copy vertex data to device local buffer
    struct Vertex {
      float position[3];
      float color[3];
    };
    {
      std::vector<Vertex> vertices = {
          {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
          {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
          {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      };
      std::vector<uint32_t> indices = {0, 1, 2};
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
    }

    // create image attachments
    uint32_t width = 1024;
    uint32_t height = 1024;
    {
      colorFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
      create2DImage(width,
                    height,
                    colorFormat_,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &color_,
                    &colorMemory_,
                    &colorView_);

      depthFormat_ = getSupportedDepthFormat();
      create2DImage(width,
                    height,
                    depthFormat_,
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
      VkAttachmentReference depthRef = {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

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
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.renderPass = renderpass_;
      bufferInfo.attachmentCount = attachments.size();
      bufferInfo.pAttachments = attachments.data();
      bufferInfo.width = width;
      bufferInfo.height = height;
      bufferInfo.layers = 1;
      CHECK_VK_SUCCESS(vkCreateFramebuffer(device_, &bufferInfo, nullptr, &framebuffer_));
    }

    // create graphics pipeline
    {
      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
      shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      shaderStages[0].module = loadShader("data/");
      shaderStages[0].pNext = "main";

      VkGraphicsPipelineCreateInfo pipeInfo{};
      pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipeInfo.stageCount = shaderStages.size();
      pipeInfo.pStages = shaderStages.data();
    }
  }

  ~HeadlessRenderer() {
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
