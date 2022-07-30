#include "VkInstance.h"
#include "vulkan/Util.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;




// Finds suitable memory for the vertex buffer
uint32_t VulkanEngine::findMemoryType( uint32_t typeFilter,
                                       VkMemoryPropertyFlags properties )
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties( m_device.getPhysicaDevice(), &memProperties );

    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
    {
        if ( ( typeFilter & ( 1 << i ) ) &&
             ( memProperties.memoryTypes[i].propertyFlags & properties ) ==
                 properties )
        {
            return i;
        }
    }
}


void VulkanEngine::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanEngine::initWindow()
{
    glfwInit();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    m_window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
}

void VulkanEngine::initVulkan()
{
    createInstance();
    createSurface();
    setupGPU();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createCommandBuffer();
    createSyncObjects();
}


VkShaderModule VulkanEngine::createShaderModule( const std::vector<char>& code )
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );
    VkShaderModule shaderModule;
    VkResult result =
        vkCreateShaderModule( m_device.getLogicalDevice(), &createInfo, nullptr, &shaderModule );
    checkSuccess( result );

    return shaderModule;
}

void VulkanEngine::createInstance()
{
    m_instance.create( true );
}


void VulkanEngine::setupGPU()
{
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    m_device.create( m_instance, deviceExtensions, m_surface.get() );
}

void VulkanEngine::createSurface()
{
    m_surface.create( m_instance, m_window );
}

void VulkanEngine::createSwapChain()
{
    VkSurfaceFormatKHR surfaceFormat{};
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    m_swapChainFormat = surfaceFormat.format;

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    int width, height;
    glfwGetFramebufferSize( m_window, &width, &height );

    m_swapChainExtent = { static_cast<uint32_t>( width ),
                          static_cast<uint32_t>( height ) };

    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_device.getPhysicaDevice(), m_surface.get(),
                                               &capabilities );
    uint32_t imageCount = capabilities.minImageCount + 1;

    /*
    * We ask for a swap chain from the graphics driver
    * We don't own the images or the memory
    * Driver talks to the Win32 API to facilitate the actual
    *   displaying of the image on the screen
    * 
    * This is probably all in VRAM, but it's likely driver 
    *   dependant
    * 
    * VkMemory is a sequence of N bytes
    * VkImage adds information about the format - allows access by 
    *   texel, or RGBA block, etc - what's in the memory?
    * VkImageView - similar  to stringView or arrayView - a view
    *   into the memory - selects a part of the underlying VkImage
    * VkFrameBuffer - binds a VkImageView with an attachment
    * VkRenderPass - defines which attachment is drawn into
    */

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface.get();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result =
        vkCreateSwapchainKHR( m_device.getLogicalDevice(), &createInfo, nullptr, &m_swapChain );
    checkSuccess( result );

    vkGetSwapchainImagesKHR( m_device.getLogicalDevice(), m_swapChain, &imageCount, nullptr );
    m_swapChainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( m_device.getLogicalDevice(), m_swapChain, &imageCount,
                             m_swapChainImages.data() );

    printf( "\nCreated Swap Chain\n" );
    printf( "\tFormat: %s\n", "VK_FORMAT_B8G8R8A8_SRGB" );
    printf( "\tColor Space: %s\n", "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR" );
    printf( "\tPresent Mode: %s\n", "VK_PRESENT_MODE_IMMEDIATE_KHR" );
    printf( "\tExtent: %d %d\n", width, height );
    printf( "\tImage Count: %d\n", imageCount );
}

void VulkanEngine::createImageViews()
{
    /*
    Create image views into the swap chain images
    */

    m_swapChainImageViews.resize( m_swapChainImages.size() );

    for ( size_t i = 0; i < m_swapChainImages.size(); i++ )
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView( m_device.getLogicalDevice(), &createInfo, nullptr,
                                             &m_swapChainImageViews[i] );
        checkSuccess( result );
    }
    printf( "\nCreated %d Image Views: %s\n",
            static_cast<int>( m_swapChainImages.size() ),
            "VK_IMAGE_VIEW_TYPE_2D" );
}

void VulkanEngine::createRenderPass()
{
    /*
    * We're giving Vulkan a "map" for how we plan to use 
        memory
    *    
    * The color attachment maps to the image in the swap chain
    *   we specify how this memory will be used
    *
    * We have one subpass, can have multiple if we're doing post-processing
    * 
    * The color attachment referenced by the subpass is directly referenced
    *   within shaders, as layout(location = 0) out vec4 outColor
    * 
    * Multiple render passes are sometimes needed, like for shadow mapping
    *   1. Offscreen render pass, from the light's point of view -> frame buffer
    *   2. Normal render pass, read the results from the frame buffer for the shadow
    *   mapping tests
    * 
    * Render pass starts
    *   Copies attachement images into framebuffer render memory
    *   Execution
    *   Copy data back to attachement images
    *   
    * During execution, the the data is indeterminate
    * You cannot use the data until the render pass is complete
    * 
    */

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;


    /*
    * The dependancy ensures the image is written into only after it's been read from
    * This allows us to run part of the pipeline while the frame is still being presented
    */

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result =
        vkCreateRenderPass( m_device.getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass );

    checkSuccess( result );

    printf( "\nCreated Render Pass\n" );
}

void VulkanEngine::createGraphicsPipeline()
{

    /*
    * Here we have a bunch of objects that define pipleline stages
    * This is just the graphics pipeline, with all programmable and fixed stages
    * 
    * Graphics pipeline object needs to know about the render pass and the subpass
    * Graphics pipeline operates on the subpass

    */

    std::cout << "\nCurrent path is: " << std::filesystem::current_path()
              << std::endl;

    auto vertShaderCode = readFile( "build/shaders/shader.vert.spv" );
    auto fragShaderCode = readFile( "build/shaders/shader.frag.spv" );

    printf( "\nLoaded shader: %s\n", "build/shaders/shader.vert.spv" );
    printf( "Loaded shader: %s\n", "build/shaders/shader.frag.spv" );

    VkShaderModule vertShaderModule = createShaderModule( vertShaderCode );
    VkShaderModule fragShaderModule = createShaderModule( fragShaderCode );

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
                                                       fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    
    //This describes the vertex data
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>( attributeDescriptions.size() );
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapChainExtent.width;
    viewport.height = (float)m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ZERO;                                        // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor =
        VK_BLEND_FACTOR_ZERO;                             // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;             // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;          // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

    VkResult result = vkCreatePipelineLayout( m_device.getLogicalDevice(), &pipelineLayoutInfo,
                                              nullptr, &m_pipelineLayout );

    checkSuccess( result );

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;  // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;  // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex = -1;               // Optional

    result = vkCreateGraphicsPipelines( m_device.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                        nullptr, &m_graphicsPipeline );

    checkSuccess( result );

    vkDestroyShaderModule( m_device.getLogicalDevice(), fragShaderModule, nullptr );
    vkDestroyShaderModule( m_device.getLogicalDevice(), vertShaderModule, nullptr );

    printf( "\nCreated Graphics Pipeline \n" );
}

void VulkanEngine::createFramebuffers()
{
    /*
    * The framebuffer binds the VkImageViews from the swap chain
    *   with the attachement that we specified in the render pass
    */

    m_swapChainFramebuffers.resize( m_swapChainImageViews.size() );

    for ( size_t i = 0; i < m_swapChainImageViews.size(); i++ )
    {
        VkImageView attachments[] = { m_swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer( m_device.getLogicalDevice(), &framebufferInfo, nullptr,
                                               &m_swapChainFramebuffers[i] );
        checkSuccess( result );
    }

    printf( "\nCreated %d Frame Buffers\n",
            static_cast<int>( m_swapChainImageViews.size() ) );
}

void VulkanEngine::createCommandPool()
{
    /*
    * Command pools allow concurrent recording of command buffers
    * One pool per thread -> Multi-thread recording
    */

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_device.getFamilyIndex();

    VkResult result =
        vkCreateCommandPool( m_device.getLogicalDevice(), &poolInfo, nullptr, &m_commandPool );

    checkSuccess( result );

    printf( "\nCreated Command Pool\n" );
}

void VulkanEngine::createVertexBuffer()
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof( m_vertices[0] ) * m_vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result =
        vkCreateBuffer( m_device.getLogicalDevice(), &bufferInfo, nullptr, &m_vertexBuffer );

    checkSuccess( result );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( m_device.getLogicalDevice(), m_vertexBuffer, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    result =
        vkAllocateMemory( m_device.getLogicalDevice(), &allocInfo, nullptr, &m_vertexBufferMemory );

    vkBindBufferMemory(m_device.getLogicalDevice(), m_vertexBuffer, m_vertexBufferMemory, 0);

    checkSuccess( result );


    void* data;
    vkMapMemory(m_device.getLogicalDevice(), m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, m_vertices.data(), (size_t) bufferInfo.size);
    vkUnmapMemory(m_device.getLogicalDevice(), m_vertexBufferMemory);
}

void VulkanEngine::createCommandBuffer()
{

    /*
    * Record work commands into this
    * Submit this to the queue for the GPU to work on
    */

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult result =
        vkAllocateCommandBuffers( m_device.getLogicalDevice(), &allocInfo, &m_commandBuffer );

    checkSuccess( result );

    printf( "\nCreated Command Buffer\n" );
}

void VulkanEngine::recordCommandBuffer( VkCommandBuffer commandBuffer,
                                      uint32_t imageIndex )
{
    /*
    * Do a render pass
    * Give it the framebuffer index -> the attachement we're drawing into   
    * Bind the graphics pipeline
    * Draw call - 3 vertices
    */

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    VkResult result = vkBeginCommandBuffer( commandBuffer, &beginInfo );

    checkSuccess( result );

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass( commandBuffer, &renderPassInfo,
                          VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                       m_graphicsPipeline );

    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );

    vkCmdDraw( commandBuffer, static_cast<uint32_t>( m_vertices.size() ), 1, 0,
               0 );

    vkCmdEndRenderPass( commandBuffer );

    result = vkEndCommandBuffer( commandBuffer );
    checkSuccess( result );

    // printf( "\nRecorded command buffer for image index: %d\n", imageIndex
    // );
}

void VulkanEngine::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateSemaphore( m_device.getLogicalDevice(), &semaphoreInfo, nullptr,
                                         &m_imageAvailableSemaphore );

    checkSuccess( result );

    result = vkCreateSemaphore( m_device.getLogicalDevice(), &semaphoreInfo, nullptr,
                                &m_renderFinishedSemaphore );
    checkSuccess( result );
    result = vkCreateFence( m_device.getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFence );
    checkSuccess( result );

    printf( "\nCreated 2 semaphores and 1 fence\n" );
}

void VulkanEngine::mainLoop()
{
    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle( m_device.getLogicalDevice() );
}

void VulkanEngine::drawFrame()
{
    /* 
    * For every frame:
    *   wait for the last frame to finish (fence)
    *   get the index of the next image
    *   record the drawing commands
    *   submit the render queue ( which will wait for the image to render into to be available - semaphore)
    *   present the image ( waiting on the rendering to be finished - semaphore )
    */

    vkWaitForFences( m_device.getLogicalDevice(), 1, &m_inFlightFence, VK_TRUE, UINT64_MAX );
    vkResetFences( m_device.getLogicalDevice(), 1, &m_inFlightFence );

    uint32_t imageIndex;
    vkAcquireNextImageKHR( m_device.getLogicalDevice(), m_swapChain, UINT64_MAX,
                           m_imageAvailableSemaphore, VK_NULL_HANDLE,
                           &imageIndex );

    vkResetCommandBuffer( m_commandBuffer, 0 );
    recordCommandBuffer( m_commandBuffer, imageIndex );

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result =
        vkQueueSubmit( m_device.getGraphicsQueue(), 1, &submitInfo, m_inFlightFence );

    checkSuccess( result );

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;  // Optional

    vkQueuePresentKHR( m_device.getGraphicsQueue(), &presentInfo );
}

void VulkanEngine::cleanup()
{
    vkDestroyBuffer(m_device.getLogicalDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(m_device.getLogicalDevice(), m_vertexBufferMemory, nullptr);

    vkDestroySemaphore( m_device.getLogicalDevice(), m_imageAvailableSemaphore, nullptr );
    vkDestroySemaphore( m_device.getLogicalDevice(), m_renderFinishedSemaphore, nullptr );
    vkDestroyFence( m_device.getLogicalDevice(), m_inFlightFence, nullptr );
    vkDestroyCommandPool( m_device.getLogicalDevice(), m_commandPool, nullptr );

    for ( auto framebuffer : m_swapChainFramebuffers )
        vkDestroyFramebuffer( m_device.getLogicalDevice(), framebuffer, nullptr );

    vkDestroyPipeline( m_device.getLogicalDevice(), m_graphicsPipeline, nullptr );
    vkDestroyPipelineLayout( m_device.getLogicalDevice(), m_pipelineLayout, nullptr );
    vkDestroyRenderPass( m_device.getLogicalDevice(), m_renderPass, nullptr );

    for ( auto imageView : m_swapChainImageViews )
        vkDestroyImageView( m_device.getLogicalDevice(), imageView, nullptr );

    vkDestroySwapchainKHR( m_device.getLogicalDevice(), m_swapChain, nullptr );

    m_device.destroy();
    m_surface.destroy( m_instance );
    m_instance.destroy();

    glfwDestroyWindow( m_window );
    glfwTerminate();
}


/*
    Swap chain gives you the images (VRAM)
    Create image views for these images
    Describe what you're planning to do with the memory - render pass - I want to draw into the image
    Describe the graphics pipeline 
    Bind the image views from the swap chain to the render pass attachment - framebuffer    
    Create the command pool and command buffer
    Record your draw commands to the command buffer
    Submit your buffer, synchronizing render and presentation


    I'm skipping in-flight frames, and swap chain re-creation - this is just rebuild everything, not that
    interesting


    For vertex buffer
    Create binding and attribute descriptors
    Tell the graphics pipeline about it
    Create the vertex buffer
    Create the buffer memory
    Memcopy into the memory
    Bind the buffer in the command recording

    Optionally use a staging buffer, then copy to GPU-side only buffer for performance

    Uniform buffers (Used for transformation matrices)
    Works similarly as vertex
    Pipeline and command recording need to know about it
    copy data into a buffer

    A better way to pass this is using push constants
    Push constants allow faster transfer of small amounts of data without using a buffer
    They're constant from the shader's perspective
    They're pushed with the command buffer data
*/
