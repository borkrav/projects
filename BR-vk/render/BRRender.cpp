#include <BRRender.h>
#include <Util.h>

#include <algorithm>
#include <cassert>
#include <ranges>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
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
}

void BRRender::initVulkan()
{
    AppState::instance().init( m_window, enableValidationLayers );

    m_device = AppState::instance().getLogicalDevice();

    m_renderPass.create( "Raster Renderpass" );
    m_pipeline.create( m_renderPass, "Raster Pipeline" );
    m_framebuffer.create( m_renderPass, "Swapchain Frame buffer" );
    m_commandPool.create( "Drawing pool" );

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
    }

    m_vertexBuffer = m_vboMgr.createBuffer( m_vertices );
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

    m_framebuffer.destroy();
    AppState::instance().recreateSwapchain();
    m_framebuffer.create( m_renderPass, "Swapchain Frame buffer" );
}

void BRRender::recordCommandBuffer( vk::CommandBuffer commandBuffer,
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

    auto clearColor = vk::ClearValue();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    commandBuffer.beginRenderPass( renderPassInfo,
                                   vk::SubpassContents::eInline );

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics,
                                m_pipeline.get() );

    vk::Buffer vertexBuffers[] = { m_vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };

    commandBuffer.bindVertexBuffers( 0, 1, vertexBuffers, offsets );
    commandBuffer.draw( static_cast<uint32_t>( m_vertices.size() ), 1, 0, 0 );
    commandBuffer.endRenderPass();

    try
    {
        commandBuffer.end();
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to record command buffer!" );
    }

    // printf( "\nRecorded command buffer for image index: %d\n", imageIndex
    // );
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

    m_commandBuffers[m_currentFrame].reset();
    recordCommandBuffer( m_commandBuffers[m_currentFrame], imageIndex );

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
    m_vboMgr.destroy();
    m_syncMgr.destroy();
    m_commandPool.destroy();
    m_framebuffer.destroy();
    m_pipeline.destroy();
    m_renderPass.destroy();

    glfwDestroyWindow( m_window );
    glfwTerminate();
}
