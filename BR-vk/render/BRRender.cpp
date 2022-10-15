#include <BRRender.h>
#include <BRUtil.h>

#include <algorithm>
#include <cassert>
#include <ranges>

#define TINYOBJLOADER_IMPLEMENTATION
#include <lodepng.h>
#include <tiny_obj_loader.h>

#define GLM_FORCE_RADIANS
#include <chrono>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;
const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

using namespace BR;

void BRRender::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void BRRender::initWindow()
{
    glfwInit();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

    m_window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );

    glfwSetWindowUserPointer( m_window, this );
    glfwSetFramebufferSizeCallback(
        m_window,
        []( GLFWwindow* window, int width, int height )
        {
            auto app = reinterpret_cast<BRRender*>(
                glfwGetWindowUserPointer( window ) );
            app->m_framebufferResized = true;
        } );

    glfwSetMouseButtonCallback(
        m_window,
        []( GLFWwindow* window, int button, int action, int mods )
        {
            auto app = reinterpret_cast<BRRender*>(
                glfwGetWindowUserPointer( window ) );
            app->onMouseButton( button, action, mods );
        } );

    glfwSetCursorPosCallback( m_window,
                              []( GLFWwindow* window, double x, double y )
                              {
                                  auto app = reinterpret_cast<BRRender*>(
                                      glfwGetWindowUserPointer( window ) );
                                  app->onMouseMove( static_cast<int>( x ),
                                                    static_cast<int>( y ) );
                              } );
}

void BRRender::onMouseButton( int button, int action, int mods )
{
    if ( action == GLFW_PRESS && !mods )
    {
        if ( button == GLFW_MOUSE_BUTTON_LEFT )
            m_modelManip.setMouseButton( ModelManip::MouseButton::lmb );
        else if ( button == GLFW_MOUSE_BUTTON_MIDDLE )
            m_modelManip.setMouseButton( ModelManip::MouseButton::mmb );
        else if ( button == GLFW_MOUSE_BUTTON_RIGHT )
            m_modelManip.setMouseButton( ModelManip::MouseButton::rmb );

        m_modelInput = true;

        double x, y;
        glfwGetCursorPos( m_window, &x, &y );
        m_modelManip.setMousePos( static_cast<int>( x ),
                                  static_cast<int>( y ) );
    }

    if ( action == GLFW_PRESS && mods & GLFW_MOD_ALT && mods & GLFW_MOD_SHIFT )
    {
        if ( button == GLFW_MOUSE_BUTTON_LEFT )
            m_cameraManip.setMouseButton( ModelManip::MouseButton::lmb );
        else if ( button == GLFW_MOUSE_BUTTON_MIDDLE )
            m_cameraManip.setMouseButton( ModelManip::MouseButton::mmb );
        else if ( button == GLFW_MOUSE_BUTTON_RIGHT )
            m_cameraManip.setMouseButton( ModelManip::MouseButton::rmb );

        m_camInput = true;

        double x, y;
        glfwGetCursorPos( m_window, &x, &y );
        m_cameraManip.setMousePos( static_cast<int>( x ),
                                   static_cast<int>( y ) );
    }

    else if ( action == GLFW_RELEASE )
    {
        m_camInput = false;
        m_modelInput = false;
    }
}

void BRRender::onMouseMove( int x, int y )
{
    if ( ImGui::GetCurrentContext() != nullptr &&
         ImGui::GetIO().WantCaptureMouse )
    {
        return;
    }
    if ( m_modelInput )
        m_modelManip.doManip(
            x, y, static_cast<ModelManip::ManipMode>( m_transformMode ) );
    else if ( m_camInput )
        m_cameraManip.doManip( x, y );
}

void BRRender::loadModel( std::string name )
{
    tinyobj::ObjReader reader;  // Used to read an OBJ file
    reader.ParseFromFile( "models/" + name );

    assert( reader.Valid() );  // Make sure tinyobj was able to parse this file
    const std::vector<tinyobj::real_t> objVertices =
        reader.GetAttrib().GetVertices();
    const std::vector<tinyobj::shape_t>& objShapes =
        reader.GetShapes();  // All shapes in the file

    std::map<std::string, int> indexMap;
    int index = 0;

    //every combination of vertex/normal pairs must be uniquely defined
    //data duplication cannot be avoided
    for ( auto shape : objShapes )
    {
        for ( auto vert : shape.mesh.indices )
        {
            auto vertIndex = vert.vertex_index;
            auto normIndex = vert.normal_index;

            BR::Pipeline::Vertex vert{
                { objVertices[vertIndex * 3], objVertices[vertIndex * 3 + 1],
                  objVertices[vertIndex * 3 + 2] },
                { objVertices[normIndex * 3], objVertices[normIndex * 3 + 1],
                  objVertices[normIndex * 3 + 2] } };

            //this might be slow
            std::string vertString =
                std::to_string( vert.pos.x ) + std::to_string( vert.pos.y ) +
                std::to_string( vert.pos.z ) + std::to_string( vert.norm.x ) +
                std::to_string( vert.norm.y ) + std::to_string( vert.norm.z );

            auto it = indexMap.find( vertString );

            if ( it == indexMap.end() )
            {
                m_vertices.push_back( vert );
                m_indices.push_back( index );
                indexMap[vertString] = index++;
            }
            else
            {
                m_indices.push_back( it->second );
            }
        }
    }

    m_vertexBuffer = m_bufferAlloc.createAndStageBuffer(
        "Vertex", m_vertices, vk::BufferUsageFlagBits::eVertexBuffer );
    m_indexBuffer = m_bufferAlloc.createAndStageBuffer(
        "Index", m_indices, vk::BufferUsageFlagBits::eIndexBuffer );
}

void BRRender::initUI()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance = AppState::instance().getInstance();
    info.PhysicalDevice = AppState::instance().getPhysicalDevice();
    info.Device = AppState::instance().getLogicalDevice();
    info.QueueFamily = AppState::instance().getFamilyIndex();
    info.Queue = AppState::instance().getGraphicsQueue();
    info.PipelineCache = nullptr;
    info.DescriptorPool = m_descriptorPool;
    info.Subpass = 1;
    info.MinImageCount = 2;
    info.ImageCount = 3;
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  // <--- need argument?
    info.CheckVkResultFn = checkSuccess;
    info.Allocator = nullptr;

    ImGui_ImplVulkan_Init( &info, m_renderPass.get() );
    ImGui_ImplGlfw_InitForVulkan( m_window, true );

    // Setup style
    ImGui::StyleColorsDark();

    auto buffer = m_commandPool.beginOneTimeSubmit( "Fonts Submit Buffer" );
    ImGui_ImplVulkan_CreateFontsTexture( buffer );
    m_commandPool.endOneTimeSubmit( buffer );
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void BRRender::initVulkan()
{
    AppState::instance().init( m_window, enableValidationLayers );

    m_device = AppState::instance().getLogicalDevice();

    loadModel( "room.obj" );

    //Descriptor set stuff (pool and UBO for transformations)
    m_descriptorPool = m_descMgr.createPool(
        "Descriptor pool", 1000,
        std::vector<BR::DescMgr::PoolSize>{
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eAccelerationStructureKHR, 1 },
            { vk::DescriptorType::eStorageImage, 1 } } );

    m_descriptorSetLayout = m_descMgr.createLayout(
        "UBO layout", std::vector<BR::DescMgr::Binding>{
                          { 0, vk::DescriptorType::eUniformBuffer, 1,
                            vk::ShaderStageFlagBits::eVertex } } );

    m_commandPool.create( "Drawing pool",
                          vk::CommandPoolCreateFlagBits::eResetCommandBuffer );

    for ( int i : std::views::iota( 0, MAX_FRAMES_IN_FLIGHT ) )
    {
        m_commandBuffers.emplace_back(
            m_commandPool.createBuffer( "Buffer for frame " + i ) );
        m_imageAvailableSemaphores.emplace_back( m_syncMgr.createSemaphore(
            "Image Avail Semaphore for frame " + i ) );
        m_renderFinishedSemaphores.emplace_back( m_syncMgr.createSemaphore(
            "Render Finish Semaphore for frame " + i ) );
        m_inFlightFences.emplace_back(
            m_syncMgr.createFence( "In Flight Fence for frame " + i ) );
        m_uniformBuffers.emplace_back( m_bufferAlloc.createUniformBuffer(
            "Uniform buffer", sizeof( UniformBufferObject ) ) );
    }

    initRaster();
    initRT();

    initUI();
}

void BRRender::createDepthBuffer()
{
    auto extent = AppState::instance().getSwapchainExtent();

    m_depthBuffer = m_bufferAlloc.createImage(
        "Depth Buffer", extent.width, extent.height, vk::Format::eD32Sfloat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal );

    m_depthBufferView = m_bufferAlloc.createImageView(
        "Depth Buffer Image View", m_depthBuffer, vk::Format::eD32Sfloat,
        vk::ImageAspectFlagBits::eDepth );
}

void BRRender::initRaster()
{
    createDepthBuffer();
    m_renderPass.create( "Raster Renderpass" );
    m_framebuffer.create( "Swapchain Frame buffer", m_renderPass,
                          m_depthBufferView );
    m_pipeline.create( "Raster Pipeline", m_renderPass, m_descriptorSetLayout );
    createDescriptorSets();
}

void BRRender::initRT()
{
    m_asBuilder.create( m_bufferAlloc );
    m_renderPass.createRT( "RT Renderpass" );
    createAS();

    m_rtDescriptorSetLayout = m_descMgr.createLayout(
        "RT Pipeline Descriptor Set Layout",
        std::vector<BR::DescMgr::Binding>{
            { 0, vk::DescriptorType::eAccelerationStructureKHR, 1,
              vk::ShaderStageFlagBits::eRaygenKHR },
            { 1, vk::DescriptorType::eStorageImage, 1,
              vk::ShaderStageFlagBits::eRaygenKHR },
            { 2, vk::DescriptorType::eUniformBuffer, 1,
              vk::ShaderStageFlagBits::eRaygenKHR } } );

    m_pipeline.createRT( "RT Pipeline", m_rtDescriptorSetLayout );
    createSBT();
    createRTDescriptorSets();
}

void BRRender::createAS()
{
    auto it = std::max_element( m_indices.begin(), m_indices.end() );
    int maxVertex = ( *it ) + 1;

    m_blas = m_asBuilder.buildBlas( "BLAS", m_vertexBuffer, m_indexBuffer,
                                    maxVertex, m_indices.size() );

    m_tlas = m_asBuilder.buildTlas( "TLAS", m_blas );
}

void BRRender::createSBT()
{
    // size, in bytes, of the shader handle
    const uint32_t handleSize =
        AppState::instance().rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAlignment =
        AppState::instance()
            .rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned =
        alignedSize( handleSize, handleSizeAlignment );
    const uint32_t groupCount =
        3;  //TODO: This is how many shaders I'm using for the RT Pipeline
    const uint32_t sbtSize = groupCount * handleSizeAligned;

    //3 32 byte handles
    std::vector<uint8_t> shaderHandleStorage( sbtSize );

    //These are the addresses for where the shaders are
    AppState::instance().vkGetRayTracingShaderGroupHandlesKHR(
        m_device, m_pipeline.getRT(), 0, groupCount, sbtSize,
        shaderHandleStorage.data() );

    const vk::BufferUsageFlags bufferUsageFlags =
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    //create 3 buffers, each holding the address of the shader
    m_raygenSBT = m_bufferAlloc.createVisibleBuffer(
        "RayGen SBT", handleSize, bufferUsageFlags,
        shaderHandleStorage.data() );
    m_missSBT = m_bufferAlloc.createVisibleBuffer(
        "Miss SBT", handleSize, bufferUsageFlags,
        shaderHandleStorage.data() + handleSizeAligned );
    m_hitSBT = m_bufferAlloc.createVisibleBuffer(
        "Hit SBT", handleSize, bufferUsageFlags,
        shaderHandleStorage.data() + handleSizeAligned * 2 );
}

void BRRender::createRTDescriptorSets()
{
    m_rtDescriptorSets.push_back( m_descMgr.createSet(
        "RT Desc Set 1", m_rtDescriptorSetLayout, m_descriptorPool ) );
    m_rtDescriptorSets.push_back( m_descMgr.createSet(
        "RT Desc Set 2", m_rtDescriptorSetLayout, m_descriptorPool ) );

    for ( int i : std::views::iota( 0, MAX_FRAMES_IN_FLIGHT ) )
    {
        //Acceleration Structure
        vk::WriteDescriptorSetAccelerationStructureKHR asInfo;
        asInfo.accelerationStructureCount = 1;
        asInfo.pAccelerationStructures = &m_tlas;

        vk::WriteDescriptorSet asWrite;
        // The specialized acceleration structure descriptor has to be chained
        asWrite.pNext = &asInfo;
        asWrite.dstSet = m_rtDescriptorSets[i];
        asWrite.dstBinding = 0;
        asWrite.descriptorCount = 1;
        asWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

        //The uniform Buffer, same as for raster
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( UniformBufferObject );

        vk::WriteDescriptorSet uniformBufferWrite;
        uniformBufferWrite.dstSet = m_rtDescriptorSets[i];
        uniformBufferWrite.dstBinding = 2;
        uniformBufferWrite.dstArrayElement = 0;
        uniformBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        uniformBufferWrite.descriptorCount = 1;

        uniformBufferWrite.pBufferInfo = &bufferInfo;
        uniformBufferWrite.pImageInfo = nullptr;        // Optional
        uniformBufferWrite.pTexelBufferView = nullptr;  // Optional

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
            asWrite, uniformBufferWrite };

        vkUpdateDescriptorSets(
            m_device, 2, (VkWriteDescriptorSet*)writeDescriptorSets.data(), 0,
            nullptr );
    }
}

void BRRender::createDescriptorSets()
{
    m_descriptorSets.push_back( m_descMgr.createSet(
        "Frame 1 Desc set", m_descriptorSetLayout, m_descriptorPool ) );
    m_descriptorSets.push_back( m_descMgr.createSet(
        "Frame 2 Desc set", m_descriptorSetLayout, m_descriptorPool ) );

    for ( int i : std::views::iota( 0, MAX_FRAMES_IN_FLIGHT ) )
    {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( UniformBufferObject );

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;        // Optional
        descriptorWrite.pTexelBufferView = nullptr;  // Optional

        vkUpdateDescriptorSets(
            m_device, 1, (VkWriteDescriptorSet*)&descriptorWrite, 0, nullptr );
    }
}

void BRRender::recreateSwapchain()
{
    //if minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize( m_window, &width, &height );
    while ( width == 0 || height == 0 )
    {
        glfwGetFramebufferSize( m_window, &width, &height );
        glfwWaitEvents();
    }

    m_device.waitIdle();

    m_bufferAlloc.free( m_depthBuffer );

    m_framebuffer.destroy();
    AppState::instance().recreateSwapchain();
    createDepthBuffer();
    m_framebuffer.create( "Swapchain Frame buffer", m_renderPass,
                          m_depthBufferView );
}

void BRRender::updateUniformBuffer( uint32_t currentImage )
{
    auto extent = AppState::instance().getSwapchainExtent();

    UniformBufferObject ubo{};

    ubo.model = m_modelManip.getMat();

    ubo.view = m_cameraManip.getMat();

    ubo.proj =
        glm::perspective( glm::radians( 45.0f ),
                          extent.width / (float)extent.height, 0.1f, 10.0f );

    ubo.proj[1][1] *= -1;

    ubo.cameraPos = m_cameraManip.getEye();

    auto mem = m_bufferAlloc.getMemory( m_uniformBuffers[currentImage] );

    void* data = m_device.mapMemory( mem, 0, sizeof( ubo ) );
    memcpy( data, &ubo, sizeof( ubo ) );
    m_device.unmapMemory( mem );

    m_asBuilder.updateTlas( m_tlas, m_blas, ubo.model );
}

void BRRender::setRTRenderTarget( uint32_t imageIndex )
{
    auto views = AppState::instance().getImageViews();

    //the render target
    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageView = views[imageIndex];
    imageInfo.imageLayout = vk::ImageLayout::eGeneral;

    vk::WriteDescriptorSet write;
    write.dstSet = m_rtDescriptorSets[m_currentFrame];
    write.dstBinding = 1;
    write.dstArrayElement = 0;
    write.descriptorType = vk::DescriptorType::eStorageImage;
    write.descriptorCount = 1;

    write.pBufferInfo = nullptr;
    write.pImageInfo = &imageInfo;     // Optional
    write.pTexelBufferView = nullptr;  // Optional

    vkUpdateDescriptorSets( m_device, 1, (VkWriteDescriptorSet*)&write, 0,
                            nullptr );
}

void BRRender::recordRasterCommandBuffer( vk::CommandBuffer commandBuffer,
                                          uint32_t imageIndex )
{
    /*
    * Do a render pass
    * Give it the framebuffer index -> the attachement we're drawing into   
    * Bind the graphics pipeline
    * Draw call - 3 vertices
    */

    auto extent = AppState::instance().getSwapchainExtent();

    auto beginInfo = vk::CommandBufferBeginInfo();

    try
    {
        commandBuffer.begin( beginInfo );
    }

    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to begin recording command buffer!" );
    }

    auto framebuffer = m_framebuffer.get();

    auto renderPassInfo = vk::RenderPassBeginInfo();
    renderPassInfo.renderPass = m_renderPass.get();
    renderPassInfo.framebuffer = framebuffer[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = extent;

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0] = vk::ClearColorValue(
        std::array<float, 4>( { { 0.2f, 0.2f, 0.2f, 0.2f } } ) );
    clearValues[1] = vk::ClearDepthStencilValue( 1.0f, 0 );

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass( renderPassInfo,
                                   vk::SubpassContents::eInline );

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics,
                                m_pipeline.get() );

    // set the dynamic state for the pipeline
    // this enables resizing of the window to work properly
    vk::Viewport viewport( 0.0f, 0.0f, extent.width, extent.height, 0.0f,
                           1.0f );
    vk::Rect2D scissor( { 0, 0 }, extent );

    commandBuffer.setViewport( 0, viewport );
    commandBuffer.setScissor( 0, scissor );

    vk::Buffer vertexBuffers[] = { m_vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };

    commandBuffer.bindVertexBuffers( 0, 1, vertexBuffers, offsets );
    commandBuffer.bindIndexBuffer( m_indexBuffer, 0, vk::IndexType::eUint16 );

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getLayout(),
        0, 1, (VkDescriptorSet*)&m_descriptorSets[m_currentFrame], 0, nullptr );

    commandBuffer.drawIndexed( static_cast<uint32_t>( m_indices.size() ), 1, 0,
                               0, 0 );

    commandBuffer.nextSubpass( vk::SubpassContents::eInline );

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), commandBuffer );

    commandBuffer.endRenderPass();

    try
    {
        commandBuffer.end();
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to record command buffer!" );
    }
}

void BRRender::recordRTCommandBuffer( vk::CommandBuffer commandBuffer,
                                      uint32_t imageIndex )
{
    auto extent = AppState::instance().getSwapchainExtent();

    auto beginInfo = vk::CommandBufferBeginInfo();

    try
    {
        commandBuffer.begin( beginInfo );
    }

    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to begin recording command buffer!" );
    }

    vk::Image srcImage = AppState::instance().getSwapchainImage( imageIndex );

    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    // memory read -> transfer read
    // undefined -> transfer source
    imageBarrier( commandBuffer, srcImage, range,
                  vk::AccessFlagBits::eMemoryRead,
                  vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eGeneral );

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eRayTracingKHR,
                                m_pipeline.getRT() );

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        m_pipeline.getRTLayout(), 0, 1,
        (VkDescriptorSet*)&m_rtDescriptorSets[m_currentFrame], 0, nullptr );

    const uint32_t handleSize =
        AppState::instance().rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAlignment =
        AppState::instance()
            .rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned =
        alignedSize( handleSize, handleSizeAlignment );

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress =
        m_bufferAlloc.getDeviceAddress( m_raygenSBT );
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress =
        m_bufferAlloc.getDeviceAddress( m_missSBT );
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress =
        m_bufferAlloc.getDeviceAddress( m_hitSBT );
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    //Ray Trace
    AppState::instance().vkCmdTraceRaysKHR(
        commandBuffer, &raygenShaderSbtEntry, &missShaderSbtEntry,
        &hitShaderSbtEntry, &callableShaderSbtEntry, extent.width,
        extent.height, 1 );

    imageBarrier( commandBuffer, srcImage, range,
                  vk::AccessFlagBits::eShaderWrite,
                  vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::eGeneral,
                  vk::ImageLayout::eGeneral );

    auto framebuffer = m_framebuffer.get();

    auto renderPassInfo = vk::RenderPassBeginInfo();
    renderPassInfo.renderPass = m_renderPass.getRT();
    renderPassInfo.framebuffer = framebuffer[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = extent;

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0] = vk::ClearColorValue(
        std::array<float, 4>( { { 0.2f, 0.2f, 0.2f, 0.2f } } ) );
    clearValues[1] = vk::ClearDepthStencilValue( 1.0f, 0 );

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues.data();

    //Render pass, this just transitions the image, doesn't clear it
    commandBuffer.beginRenderPass( renderPassInfo,
                                   vk::SubpassContents::eInline );

    //Draw the UI over the RT output
    commandBuffer.nextSubpass( vk::SubpassContents::eInline );

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), commandBuffer );

    commandBuffer.endRenderPass();

    try
    {
        commandBuffer.end();
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to record command buffer!" );
    }
}

void BRRender::drawUI()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin( "Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize );
    ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)",
                 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
    ImGui::Checkbox( "Ray Tracing", &m_rtMode );

    const char* items[] = { "Rotate", "Translate", "Scale" };
    ImGui::Combo( "Model Manip", &m_transformMode, items,
                  IM_ARRAYSIZE( items ) );

    if ( ImGui::Button( "Reset Transforms" ) )
    {
        m_modelManip.reset();
        m_cameraManip.reset();
    }

    if ( ImGui::Button( "Screenshot" ) )
    {
        takeScreenshot();
    }
    ImGui::End();
}

void BRRender::drawFrame()
{
    /* 
    * For every frame:
    *   wait for the last frame to finish (fence)
    *   get the index of the next image
    *   record the drawing commands
    *   submit the render queue ( which will wait for the image to render into to be available - semaphore)
    *   present the image ( waiting on the rendering to be finished - semaphore )
    */

    auto queue = AppState::instance().getGraphicsQueue();

    auto result =
        m_device.waitForFences( 1, &m_inFlightFences[m_currentFrame], VK_TRUE,
                                std::numeric_limits<uint64_t>::max() );

    uint32_t imageIndex;

    try
    {
        result = m_device.acquireNextImageKHR(
            AppState::instance().getSwapchain(),
            std::numeric_limits<uint64_t>::max(),
            m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE,
            &imageIndex );
    }
    catch ( vk::OutOfDateKHRError err )
    {
        recreateSwapchain();
        return;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to acquire swap chain image!" );
    }

    // Only reset the fence if we are submitting work
    result = m_device.resetFences( 1, &m_inFlightFences[m_currentFrame] );

    //if (!m_rtMode)
    drawUI();

    updateUniformBuffer( m_currentFrame );

    if ( m_rtMode )
        setRTRenderTarget( imageIndex );

    m_commandBuffers[m_currentFrame].reset();

    if ( !m_rtMode )
        recordRasterCommandBuffer( m_commandBuffers[m_currentFrame],
                                   imageIndex );

    else
        recordRTCommandBuffer( m_commandBuffers[m_currentFrame], imageIndex );

    auto submitInfo = vk::SubmitInfo();

    vk::Semaphore waitSemaphores[] = {
        m_imageAvailableSemaphores[m_currentFrame] };
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    vk::Semaphore signalSemaphores[] = {
        m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    try
    {
        queue.submit( submitInfo, m_inFlightFences[m_currentFrame] );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to submit draw command buffer!" );
    }

    auto presentInfo = vk::PresentInfoKHR();

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = { AppState::instance().getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;  // Optional

    vk::Result resultPresent;
    try
    {
        resultPresent = queue.presentKHR( presentInfo );
    }
    catch ( vk::OutOfDateKHRError err )
    {
        resultPresent = vk::Result::eErrorOutOfDateKHR;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to present swap chain image!" );
    }

    if ( resultPresent == vk::Result::eSuboptimalKHR ||
         resultPresent == vk::Result::eSuboptimalKHR || m_framebufferResized )
    {
        m_framebufferResized = false;
        recreateSwapchain();
        return;
    }

    m_currentFrame = ( m_currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

void BRRender::takeScreenshot()
{
    printf( "taking screenshot!\n" );

    auto extent = AppState::instance().getSwapchainExtent();
    auto format = AppState::instance().getSwapchainFormat();

    vk::Image srcImage =
        AppState::instance().getSwapchainImage( m_currentFrame );

    // Create Destination image for the screenshot, linear tiling (regular array, so I can copy out)
    // Layouts is how the image is stored in VRAM, need to select the optimal layout for the operation
    // Layouts are probably related to internal compression, where the hardware compresses the image to save
    //      bandwidth - need to explicitely indicate that we plan to do operation X to the image, so it can be
    //      properly accessed
    // Basically, we tell the driver what we're planning to do with the image, and the driver ensures that
    //      those operations are optimal, by changing the layout. What this means is up to the driver/GPU

    vk::Image dstImage = m_bufferAlloc.createImage(
        "Screenshot Destination Image", extent.width, extent.height,
        vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eLinear,
        vk::ImageUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent );

    vk::DeviceMemory dstMem = m_bufferAlloc.getMemory( dstImage );

    auto cmdBuff =
        m_commandPool.beginOneTimeSubmit( "Screenshot Command Buffer" );

    //asking for color
    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    // none -> transfer write
    // undefined -> optimal transfer destination
    imageBarrier( cmdBuff, dstImage, range, vk::AccessFlagBits::eNone,
                  vk::AccessFlagBits::eTransferWrite,
                  vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eTransferDstOptimal );

    // memory read -> transfer read
    // presentation -> transfer source
    imageBarrier( cmdBuff, srcImage, range, vk::AccessFlagBits::eMemoryRead,
                  vk::AccessFlagBits::eTransferRead,
                  vk::ImageLayout::ePresentSrcKHR,
                  vk::ImageLayout::eTransferSrcOptimal );

    //copy the color, width*height data
    vk::ImageCopy imageCopy;
    imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopy.srcSubresource.layerCount = 1;
    imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopy.dstSubresource.layerCount = 1;
    imageCopy.extent.width = extent.width;
    imageCopy.extent.height = extent.height;
    imageCopy.extent.depth = 1;

    //do the actual copy
    cmdBuff.copyImage( srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage,
                       vk::ImageLayout::eTransferDstOptimal, imageCopy );

    // transfer write -> memory read
    // transfer destination -> general
    imageBarrier( cmdBuff, dstImage, range, vk::AccessFlagBits::eTransferWrite,
                  vk::AccessFlagBits::eMemoryRead,
                  vk::ImageLayout::eTransferDstOptimal,
                  vk::ImageLayout::eGeneral );

    // transfer read -> memory read
    // transfer source -> presentation
    imageBarrier( cmdBuff, srcImage, range, vk::AccessFlagBits::eTransferRead,
                  vk::AccessFlagBits::eMemoryRead,
                  vk::ImageLayout::eTransferSrcOptimal,
                  vk::ImageLayout::ePresentSrcKHR );

    m_commandPool.endOneTimeSubmit( cmdBuff );

    //get layout
    vk::ImageSubresource subResource( vk::ImageAspectFlagBits::eColor, 0, 0 );
    auto subResourceLayout =
        m_device.getImageSubresourceLayout( dstImage, subResource );

    //map image to host memory
    const char* data =
        (const char*)m_device.mapMemory( dstMem, 0, VK_WHOLE_SIZE );

    data += subResourceLayout.offset;

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface formats for demonstation purposes

    std::vector<vk::Format> formatsBGR = { vk::Format::eB8G8R8A8Srgb,
                                           vk::Format::eB8G8R8A8Unorm,
                                           vk::Format::eB8G8R8A8Snorm };

    colorSwizzle = ( std::find( formatsBGR.begin(), formatsBGR.end(),
                                format ) != formatsBGR.end() );

    //fill the image vector
    std::vector<unsigned char> image;

    uint64_t w = extent.width;
    uint64_t h = extent.height;

    image.resize( w * h * 4 );

    for ( uint64_t y = 0; y < h; y++ )
    {
        //go through data byte by byte
        unsigned char* row = (unsigned char*)data;
        for ( uint64_t x = 0; x < w; x++ )
        {
            if ( colorSwizzle )
            {
                image[4 * w * y + 4 * x + 0] = *( row + 2 );
                image[4 * w * y + 4 * x + 1] = *( row + 1 );
                image[4 * w * y + 4 * x + 2] = *( row + 0 );
                image[4 * w * y + 4 * x + 3] = 255;
            }

            else
            {
                image[4 * w * y + 4 * x + 0] = *( row + 0 );
                image[4 * w * y + 4 * x + 1] = *( row + 1 );
                image[4 * w * y + 4 * x + 2] = *( row + 2 );
                image[4 * w * y + 4 * x + 3] = 255;
            }

            //increment by 4 bytes (RGBA)
            row += 4;
        }
        //go to next row
        data += subResourceLayout.rowPitch;
    }

    //create filename, with current date and time
    auto p = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t( p );
    char str[26];
    ctime_s( str, sizeof str, &t );
    std::string fileName = "screenshots/" + (std::string)str + ".png";

    //clean up the string
    fileName.erase( std::remove( fileName.begin(), fileName.end(), '\n' ),
                    fileName.end() );
    std::replace( fileName.begin(), fileName.end(), ' ', '-' );
    std::replace( fileName.begin(), fileName.end(), ':', '-' );

    //encode to PNG
    unsigned error = lodepng::encode( fileName, image.data(), w, h );

    assert( error == 0 );

    m_device.unmapMemory( dstMem );
    m_bufferAlloc.free( dstImage );
}

void BRRender::mainLoop()
{
    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents();
        drawFrame();
    }
    //takeScreenshot();

    vkDeviceWaitIdle( AppState::instance().getLogicalDevice() );
}

void BRRender::cleanup()
{
    m_asBuilder.destroy();
    m_bufferAlloc.destroy();
    m_syncMgr.destroy();
    m_commandPool.destroy();
    m_framebuffer.destroy();
    m_pipeline.destroy();
    m_renderPass.destroy();
    m_descMgr.destroy();

    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow( m_window );
    glfwTerminate();
}
