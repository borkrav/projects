#include <BRAppState.h>
#include <BRPipeline.h>
#include <BRUtil.h>

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace BR;

vk::ShaderModule createShaderModule( vk::Device device,
                                     const std::vector<char>& code )
{
    auto createInfo = vk::ShaderModuleCreateInfo();
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );

    try
    {
        vk::ShaderModule shaderModule = device.createShaderModule( createInfo );
        return shaderModule;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create shader module!" );
    }
}

Pipeline::Pipeline()
    : m_graphicsPipeline( nullptr ), m_pipelineLayout( nullptr )
{
}

Pipeline::~Pipeline()
{
    assert( !m_graphicsPipeline && !m_pipelineLayout );
}

void Pipeline::create( std::string name, RenderPass& renderpass,
                       vk::DescriptorSetLayout layout )
{
    /*
    * Here we have a bunch of objects that define pipleline stages
    * This is just the graphics pipeline, with all programmable and fixed stages
    * 
    * Graphics pipeline object needs to know about the render pass and the subpass
    * Graphics pipeline operates on the subpass

    */

    std::cout << "\nCurrent load path is: " << std::filesystem::current_path()
              << std::endl;

    auto vertShaderCode = readFile( "build/shaders/shader.vert.spv" );
    auto fragShaderCode = readFile( "build/shaders/shader.frag.spv" );

    m_device = AppState::instance().getLogicalDevice();
    auto swapChainExtent = AppState::instance().getSwapchainExtent();

    vk::ShaderModule vertShaderModule =
        createShaderModule( m_device, vertShaderCode );
    vk::ShaderModule fragShaderModule =
        createShaderModule( m_device, fragShaderCode );

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        { vk::PipelineShaderStageCreateFlags(),
          vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main" },
        { vk::PipelineShaderStageCreateFlags(),
          vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main" } };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>( attributeDescriptions.size() );
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapChainExtent;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    try
    {
        m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create pipeline layout!" );
    }

    DEBUG_NAME( m_pipelineLayout, name + "layout" );

    std::vector<vk::DynamicState> dynamicStates{ vk::DynamicState::eScissor,
                                                 vk::DynamicState::eViewport };

    vk::PipelineDynamicStateCreateInfo dynamicInfo;
    dynamicInfo.dynamicStateCount = 2;
    dynamicInfo.pDynamicStates = dynamicStates.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderpass.get();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.pDynamicState = &dynamicInfo;

    try
    {
        auto pipe = m_device.createGraphicsPipeline( nullptr, pipelineInfo );
        m_graphicsPipeline = pipe;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create graphics pipeline!" );
    }

    m_device.destroyShaderModule( vertShaderModule );
    m_device.destroyShaderModule( fragShaderModule );

    DEBUG_NAME( m_graphicsPipeline, name )
}

void Pipeline::createRT( std::string name,
                         vk::DescriptorSetLayout descriptorSetLayout )
{
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    m_rtPipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );
    DEBUG_NAME( m_rtPipelineLayout, "RT Pipeline Layout" );

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

    auto raygenCode = readFile( "build/shaders/raygen.rgen.spv" );
    auto missCode = readFile( "build/shaders/miss.rmiss.spv" );
    auto closesthitCode = readFile( "build/shaders/closesthit.rchit.spv" );

    m_device = AppState::instance().getLogicalDevice();

    vk::ShaderModule raygenModule = createShaderModule( m_device, raygenCode );
    vk::ShaderModule missModule = createShaderModule( m_device, missCode );
    vk::ShaderModule closesthitModule =
        createShaderModule( m_device, closesthitCode );

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        { vk::PipelineShaderStageCreateFlags(),
          vk::ShaderStageFlagBits::eRaygenKHR, raygenModule, "main" },
        { vk::PipelineShaderStageCreateFlags(),
          vk::ShaderStageFlagBits::eMissKHR, missModule, "main" },
        { vk::PipelineShaderStageCreateFlags(),
          vk::ShaderStageFlagBits::eClosestHitKHR, closesthitModule, "main" } };

    // Ray generation group
    {
        vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
        shaderGroup.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        shaderGroup.generalShader = 0;
        shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back( shaderGroup );
    }

    // Miss group
    {
        vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
        shaderGroup.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        shaderGroup.generalShader = 1;
        shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back( shaderGroup );
    }

    // Closest hit group
    {
        vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
        shaderGroup.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
        shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.closestHitShader = 2;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back( shaderGroup );
    }

    vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineInfo;
    rayTracingPipelineInfo.stageCount = 3;
    rayTracingPipelineInfo.pStages = shaderStages;
    rayTracingPipelineInfo.groupCount =
        static_cast<uint32_t>( shaderGroups.size() );
    rayTracingPipelineInfo.pGroups = shaderGroups.data();
    rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineInfo.layout = m_rtPipelineLayout;

    auto result = AppState::instance().vkCreateRayTracingPipelinesKHR(
        m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
        reinterpret_cast<VkRayTracingPipelineCreateInfoKHR*>(
            &rayTracingPipelineInfo ),
        nullptr, reinterpret_cast<VkPipeline*>( &m_rtPipeline ) );

    checkSuccess( result );

    DEBUG_NAME( m_rtPipeline, name );

    m_device.destroyShaderModule( raygenModule );
    m_device.destroyShaderModule( missModule );
    m_device.destroyShaderModule( closesthitModule );
}

void Pipeline::destroy()
{
    m_device.destroyPipeline( m_graphicsPipeline );
    m_device.destroyPipelineLayout( m_pipelineLayout );

    m_device.destroyPipeline( m_rtPipeline );
    m_device.destroyPipelineLayout( m_rtPipelineLayout );

    m_graphicsPipeline = nullptr;
    m_pipelineLayout = nullptr;
}

vk::Pipeline Pipeline::get()
{
    assert( m_graphicsPipeline );
    return m_graphicsPipeline;
}

vk::Pipeline Pipeline::getRT()
{
    assert( m_rtPipeline );
    return m_rtPipeline;
}

vk::PipelineLayout Pipeline::getLayout()
{
    assert( m_pipelineLayout );
    return m_pipelineLayout;
}

vk::PipelineLayout Pipeline::getRTLayout()
{
    assert( m_rtPipelineLayout );
    return m_rtPipelineLayout;
}