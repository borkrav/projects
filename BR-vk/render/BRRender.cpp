#include <BRRender.h>
#include <BRUtil.h>
#include <lodepng.h>

#include <algorithm>
#include <cassert>
#include <ranges>

#define GLM_FORCE_RADIANS
#include <chrono>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

using namespace BR;

BRRender::BRRender()
    : m_window( AppState::instance().getWindow() ),
      m_descMgr( AppState::instance().getDescMgr() ),
      m_framesInFlight( AppState::instance().m_framesInFlight ),
      m_syncMgr( AppState::instance().getSyncMgr() ),
      m_bufferAlloc( AppState::instance().getMemoryMgr() )
{
}

void BRRender::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void BRRender::initWindow()
{
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
    {
        m_modelManip.doManip(
            x, y, static_cast<ModelManip::ManipMode>( m_transformMode ) );
        m_iteration = 0;
    }
    else if ( m_camInput )
    {
        m_cameraManip.doManip( x, y );
        m_iteration = 0;
    }
}

void BRRender::createUIRenderPass()
{
    auto format = AppState::instance().getSwapchainFormat();

    m_renderPass.addAttachment(
        format, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
        vk::ImageLayout::eColorAttachmentOptimal );

    m_renderPass.addAttachment(
        vk::Format::eD32Sfloat, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::ImageLayout::eDepthStencilAttachmentOptimal );

    m_renderPass.addSubpass( vk::PipelineBindPoint::eGraphics, 0, 1 );

    m_renderPass.addDependency(
        VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eMemoryRead,
        vk::AccessFlagBits::eColorAttachmentRead |
            vk::AccessFlagBits::eColorAttachmentWrite );

    m_renderPass.build( "UI Renderpass" );
}

void BRRender::initUI()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    createUIRenderPass();

    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance = AppState::instance().getInstance();
    info.PhysicalDevice = AppState::instance().getPhysicalDevice();
    info.Device = AppState::instance().getLogicalDevice();
    info.QueueFamily = AppState::instance().getFamilyIndex();
    info.Queue = AppState::instance().getGraphicsQueue();
    info.PipelineCache = nullptr;
    info.DescriptorPool = m_descriptorPool;
    info.Subpass = 0;
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
    m_device = AppState::instance().getLogicalDevice();

    // No normals
    // m_scene.loadModel( "CasualEffects/bedroom/iscv2.obj" ); 
    // m_scene.loadModel( "CasualEffects/cloud/cumulus00.obj" );
    // m_scene.loadModel( "CasualEffects/geodesic/geodesic_classI_20.obj" );
    // m_scene.loadModel( "CasualEffects/lpshead/head.obj" );
    // m_scene.loadModel( "CasualEffects/sibenik/sibenik.obj" );
    // m_scene.loadModel( "CasualEffects/sponza2/sponza.obj" );
    // m_scene.loadModel( "CasualEffects/teapot/teapot.obj" );

    // Works
    // m_scene.loadModel( "CasualEffects/bmw/bmw.obj" );
    // m_scene.loadModel( "CasualEffects/breakfast_room/breakfast_room.obj" );
    // m_scene.loadModel( "CasualEffects/buddha/buddha.obj" );
    // m_scene.loadModel( "CasualEffects/bunny/bunny.obj" );
    // m_scene.loadModel( "CasualEffects/chestnut/AL05a.obj" );
    // m_scene.loadModel( "CasualEffects/conference/conference.obj" );
    
    //m_scene.loadModel( "CasualEffects/CornellBox/CornellBox-Water.obj" );
    
    // m_scene.loadModel( "CasualEffects/Cube/Cube.obj" );
    // m_scene.loadModel( "CasualEffects/dragon/dragon.obj" );
    // m_scene.loadModel( "CasualEffects/erato/erato.obj" );
     m_scene.loadModel( "CasualEffects/bistro/Exterior/exterior.obj" );
    // m_scene.loadModel( "CasualEffects/bistro/Interior/interior.obj" );
    // m_scene.loadModel( "CasualEffects/fireplace_room/fireplace_room.obj" );
    // m_scene.loadModel( "CasualEffects/gallery/gallery.obj" );
    // m_scene.loadModel( "CasualEffects/hairball/hairball.obj" );
    // m_scene.loadModel( "CasualEffects/holodeck/holodeck.obj" );
    // m_scene.loadModel( "CasualEffects/living_room/living_room.obj" );
    // m_scene.loadModel( "CasualEffects/lost-empire/lost_empire.obj" );
    // m_scene.loadModel( "CasualEffects/mitsuba/mitsuba.obj" );
    // m_scene.loadModel( "CasualEffects/mori_knob/testObj.obj" );
    // m_scene.loadModel( "CasualEffects/pine/scrubPine.obj" );
    // m_scene.loadModel( "CasualEffects/powerplant/powerplant.obj" );
    // m_scene.loadModel( "CasualEffects/roadBike/roadBike.obj" );
    // m_scene.loadModel( "CasualEffects/rungholt/house.obj" );
    // m_scene.loadModel( "CasualEffects/rungholt/rungholt.obj" );
    // m_scene.loadModel( "CasualEffects/salle_de_bain/salle_de_bain.obj" );
    // m_scene.loadModel( "CasualEffects/San_Miguel/san-miguel.obj" );
    // m_scene.loadModel( "CasualEffects/sportsCar/sportsCar.obj" );
    // m_scene.loadModel( "CasualEffects/sphere/sphere-cubecoords.obj" );
    // m_scene.loadModel( "CasualEffects/sponza/sponza.obj" );
    // m_scene.loadModel( "CasualEffects/vokselia_spawn/vokselia_spawn.obj" );
    // m_scene.loadModel( "CasualEffects/white_oak/white_oak.obj" );
    
    // m_scene.loadModel( "Bugatti_type_35b.obj" );
     
    //Descriptor set stuff (pool and UBO for transformations)
    m_descriptorPool = m_descMgr.createPool(
        "Descriptor pool", 1000,
        std::vector<BR::DescMgr::PoolSize>{
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eAccelerationStructureKHR, 1 },
            { vk::DescriptorType::eStorageImage, 1 } } );

    for ( int i : std::views::iota( 0, m_framesInFlight ) )
    {
        m_uniformBuffers.emplace_back( m_bufferAlloc.createDeviceBuffer(
            "Uniform buffer", sizeof( UniformBufferObject ), nullptr, true,
            vk::BufferUsageFlagBits::eUniformBuffer ) );
    }

    m_raster.init();
    m_raster.createDescriptorSets( m_uniformBuffers, m_descriptorPool );

    m_commandPool.create( "Drawing pool",
                          vk::CommandPoolCreateFlagBits::eResetCommandBuffer );

    for ( int i : std::views::iota( 0, m_framesInFlight ) )
    {
        m_commandBuffers.emplace_back(
            m_commandPool.createBuffer( "Buffer for frame " + i ) );
        m_imageAvailableSemaphores.emplace_back( m_syncMgr.createSemaphore(
            "Image Avail Semaphore for frame " + i ) );
        m_renderFinishedSemaphores.emplace_back( m_syncMgr.createSemaphore(
            "Render Finish Semaphore for frame " + i ) );
        m_inFlightFences.emplace_back(
            m_syncMgr.createFence( "In Flight Fence for frame " + i ) );
    }

    m_raytracer.init();
    m_raytracer.createAS( m_scene.m_indices, m_scene.m_rtVertexBuffer,
                          m_scene.m_indexBuffer );
    m_raytracer.createSBT();
    m_raytracer.createRTDescriptorSets( m_uniformBuffers, m_descriptorPool,
                                        m_scene.m_rtVertexBuffer,
                                        m_scene.m_indexBuffer );

    initUI();
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

    AppState::instance().recreateSwapchain();

    m_raster.resize();
    m_raytracer.resize();

    m_iteration = 0;
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
    ubo.iteration = ++m_iteration;
    ubo.accumulate = m_rtAccumulate;
    ubo.mode = m_rtType;

    m_bufferAlloc.updateVisibleBuffer( m_uniformBuffers[currentImage],
                                       sizeof( ubo ), &ubo );

    m_raytracer.updateTLAS( ubo.model );
}

void BRRender::drawUI()
{
    bool oldAcc = m_rtAccumulate;
    int oldType = m_rtType;

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin( "Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize );
    ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)",
                 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
    ImGui::Checkbox( "Ray Tracing", &m_rtMode );
    ImGui::Checkbox( "Accumulation", &m_rtAccumulate );

    const char* items[] = { "Rotate", "Translate", "Scale" };
    ImGui::Combo( "Model Manip", &m_transformMode, items,
                  IM_ARRAYSIZE( items ) );

    const char* rtItems[] = { "Mirror", "Glossy", "Sharp", "AO" };
    ImGui::Combo( "RT mode", &m_rtType, rtItems, IM_ARRAYSIZE( rtItems ) );

    if ( ImGui::Button( "Reset Transforms" ) )
    {
        m_modelManip.reset();
        m_cameraManip.reset();
    }

    if ( ImGui::Button( "Screenshot" ) )
    {
        AppState::instance().takeScreenshot( m_commandPool, m_currentFrame );
    }
    ImGui::End();

    if ( oldAcc != m_rtAccumulate || oldType != m_rtType )
        m_iteration = 0;
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

    drawUI();

    updateUniformBuffer( m_currentFrame );

    if ( m_rtMode )
        m_raytracer.setRTRenderTarget( imageIndex, m_currentFrame );

    auto commandBuffer = m_commandBuffers[m_currentFrame];

    commandBuffer.reset();

    try
    {
        commandBuffer.begin( vk::CommandBufferBeginInfo() );
    }

    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to begin recording command buffer!" );
    }

    if ( !m_rtMode )
    {
        m_raster.recordDrawCommandBuffer(
            m_commandBuffers[m_currentFrame], imageIndex, m_currentFrame,
            m_scene.m_vertexBuffer, m_scene.m_indexBuffer,
            m_scene.m_indices.size() );
    }

    else
    {
        m_raytracer.recordRTCommandBuffer( m_commandBuffers[m_currentFrame],
                                           imageIndex, m_currentFrame,
                                           m_raster );
    }

    //The Raster creates the framebuffer object
    //The Raster has one renderpass
    //The UI has it's own renderpass
    //The UI renderpass is compatable with  the Raster's framebuffer object, therefore
    //We can use the same framebuffer object
    //If a render pass is not compatable with a framebuffer (different attachments)
    //We have to create more framebuffers

    auto& framebuffer = m_raster.getFrameBuffer().get();
    auto extent = AppState::instance().getSwapchainExtent();

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

    m_currentFrame = ( m_currentFrame + 1 ) % m_framesInFlight;
}

void BRRender::mainLoop()
{
    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle( AppState::instance().getLogicalDevice() );
}

void BRRender::cleanup()
{
    m_commandPool.destroy();
    m_raster.destroy();
    m_raytracer.destroy();
    m_renderPass.destroy();

    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow( m_window );
    glfwTerminate();
}
