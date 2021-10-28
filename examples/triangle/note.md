# Note

## process
main()
    VulkanExample()
        VulkanExampleBase()
            check data folder exsits
            change settings.validation
            parse command line
            init xcb connection
    setup camera {lootAt, position, rotation, perspective}
    initValkan()
        createInstance()
        set validation layer
        set VkApplicationInfo
        set VkInstanceCreateInfo {appInfo, extensions{surface, xcb}, layers{validation}}
        vkCreateInstance
        select physicalDevice
		initialize VulkanDevice (physicalDevice)
            get physical device properties, features, memory properties
            get queue family properties
            get device extensions
        vulkanDevice->createLogicalDevice()
            set VkDeviceQueueCreateInfos {graphics, compute, transfer}
            set queue family indices
            set deviceExtensions
            set VkDeviceCreateInfo {queueCreateInfos, enabledFeatures, enabledExtensions}
            vkCreateDevice
            createCommandPool (graphicsQueueIndex, reset)
        vkGetDeviceQueue
        swapChain.connect(instance, physicalDevice, device)
            set swapChain related function pointers
        createSemaphores {presentComplete, renderComplete}
        set submitInfo {waitDstStageMask=OUTPUT, waitSemaphores=presentComplete, signalSemaphore=renderComplete}
    setupWindow()
        xcb_generate_id
        xcb_create_windoe
        xcb_change_property
        xcb_map_window
    prepare()
        VulkanExampleBase::prepare()
            initSwapchain()
                swapchain.initSurface()
                    set VkXcbSurfaceCreateInfo {connection, window}
                    search a queue support graphics and present
                    get supported surface format, prefer B8G8R8A8_UNORM
            createCommandPool() for queue present
            setupSwapChain()
                get physical device surface properties and formats
                get available present mode
                set swapchain width and height
                set swapchain present mode {mailbox, immediate, fifo}
                set surface transform identity
                find supported alpha format
                set VkSwapchainCreateInfo {surface, minImageCount, colorFormat, colorSpace, imageExtent, ...}
                vkCreateSwapChainKHR()
                get swap chain VkImages
                create VkImageViews, binding to VkImages
                output swapchain width and height
            createCommandBuffers()
                #buffer = #imageCount
            createSynchronizationPrimitives()
                createFences #fence = #buffer
            setupDepthStencil()
                set VkImageCreateInfo depth_stencil
                vkCreateImage()
                allocate memory, bind image memory
                set VkImageViewCreateInfo for depth_stencil
                vkCreateImageView
            setupRenderPass()
                set VkAttachmentDescription of color and depth_stencil
                set VkAttachmentReference of color and depth_stencil
                set VkSubpassDescription 
                set VkSubpassDependency of color and depth_stencil
            createPipelineCache()
            setupFrameBuffer()
                set VkFramebufferCreateInfo
                vkCreateFramebuffer()
            setOverlay
        prepareSynchronizationPrimitives()
            create fences #fences = #cmdBuffers
            create presentComplete & renderComplete semaphores
        prepareVertices()
            set VertexBuffer 
            set IndexBuffer
            copy data from local to host local memory for optimial access by GPU
                vkCreateBuffer -> vkGetBufferMemoryRequirements -> getMemoryTypeIndex -> vkAllocateMemory -> vkMapMemory -> memcpy -> vkUnmapMemory -> vkBindBufferMemory
                create CommandBuffer -> vkCmdCopyBuffer -> vkEndCommandBuffer -> Submit -> WaitFence
                destropy staging buffer
        prepareUniformBuffers()
            createBuffer -> getMemReq -> getMemIndex -> AllocateMem -> BindBufferMem
            UpdateUniformBuffers()
                mapMem -> memcpy -> unmapMem
        setupDescriptorSetLayout()
            set VkDescriptorSetLayoutBinding
            set VkDescriptorSetLayoutCreateInfo
            vkCreateDescriptorSetLayout
            set VkPipelineLayoutCreateInfo
            vkCreatePipelineLayout
        preparePipeline()
            VkGraphicsPipelineCreateInfo
            VkPipelineInputAssemblyStateCreateInfo
            VkPipelineRasterizationStateCreateInfo
            VkPipelineColorBlendAttachmentState
            VkPipelineColorBlendStateCreateInfo
            VkPipelineViewportStateCreateInfo
            VkDynamicState
            VkPipelineDynamicStateCreateInfo
            VkPipelineDepthStencilStateCreateInfo
            VkPipelineMultisampleStateCreateInfo
            VkVertexInputBindingDescription
            VkPipelineVertexInputStateCreateInfo
            VkPipelineShaderStageCreateInfo
            vkCreateGraphicsPipelines
            vkDestroyShaderModule
        setupDescriptorPool()
            VkDescriptorPoolCreateInfo
            vkCreateDescriptorPool
        setupDescriptorSet()
            VkDescriptorAllocateInfo -> vkAllocateDescriptorSets
            VkWriteDescriptorSet -> vkUpdateDescriptorSets
        buildCommandBuffers()
            vkBeginCommandBuffer
            vkCmdBeginRenderPass
            vkCmdSetViewport
            vkCmdSetScissor
            vkCmdBindDescriptorSets
            vkCmdBindPipeline
            vkCmdBindVertexBuffers
            vkCmdBindIndexBuffer
            vkCmdDrawIndexed
            vkCmdEndRenderPass
            vkEndCommandBuffer
    renderLoop()
        draw()
            AccquireNextImageKHR(presentCompleteSemaphore)
            waitFence, resetFence
            submitInfo -> queueSubmit
            presentInfo -> queuePresentKHR (finally get image)
