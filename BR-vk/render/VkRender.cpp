#include "VkRender.h"

#include "Util.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;



void VkRender::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VkRender::initWindow()
{
    glfwInit();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    m_window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
}

void VkRender::initVulkan()
{
    m_instance.create( true );
    m_surface.create( m_instance, m_window );

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    m_device.create( m_instance, deviceExtensions, m_surface );
    m_swapchain.create( m_window, m_device, m_surface );
    m_swapchain.createImageViews( m_device );
    m_renderPass.create( m_device, m_swapchain );
    m_pipeline.create( m_device, m_swapchain, m_renderPass );
    m_framebuffer.create( m_swapchain, m_renderPass, m_device );
    m_commandPool.create( m_device );
    m_commandPool.createBuffer( m_device );

    m_imageAvailableSemaphore = m_syncMgr.createSemaphore( m_device );
    m_renderFinishedSemaphore = m_syncMgr.createSemaphore( m_device );
    m_inFlightFence = m_syncMgr.createFence( m_device );

     m_vertexBuffer = m_vboMgr.createBuffer( m_vertices, m_device );
}


void VkRender::recordCommandBuffer( VkCommandBuffer commandBuffer,
                                        uint32_t imageIndex )
{
    /*
    * Do a render pass
    * Give it the framebuffer index -> the attachement we're drawing into   
    * Bind the graphics pipeline
    * Draw call - 3 vertices
    */

    auto extent = m_swapchain.getExtent();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    VkResult result = vkBeginCommandBuffer( commandBuffer, &beginInfo );

    checkSuccess( result );

    auto framebuffer = m_framebuffer.get();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass.get();
    renderPassInfo.framebuffer = framebuffer[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass( commandBuffer, &renderPassInfo,
                          VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                       m_pipeline.get() );

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


void VkRender::mainLoop()
{
    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle( m_device.getLogicalDevice() );
}

void VkRender::drawFrame()
{
    /* 
    * For every frame:
    *   wait for the last frame to finish (fence)
    *   get the index of the next image
    *   record the drawing commands
    *   submit the render queue ( which will wait for the image to render into to be available - semaphore)
    *   present the image ( waiting on the rendering to be finished - semaphore )
    */

    vkWaitForFences( m_device.getLogicalDevice(), 1, &m_inFlightFence, VK_TRUE,
                     UINT64_MAX );
    vkResetFences( m_device.getLogicalDevice(), 1, &m_inFlightFence );

    uint32_t imageIndex;
    vkAcquireNextImageKHR( m_device.getLogicalDevice(), m_swapchain.get(),
                           UINT64_MAX, m_imageAvailableSemaphore,
                           VK_NULL_HANDLE, &imageIndex );

    auto buffer = m_commandPool.getBuffer();

    vkResetCommandBuffer( buffer, 0 );
    recordCommandBuffer( buffer, imageIndex );

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result = vkQueueSubmit( m_device.getGraphicsQueue(), 1,
                                     &submitInfo, m_inFlightFence );

    checkSuccess( result );

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapchain.get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;  // Optional

    vkQueuePresentKHR( m_device.getGraphicsQueue(), &presentInfo );
}

void VkRender::cleanup()
{
    m_vboMgr.destroy( m_device );
    m_syncMgr.destroy( m_device );
    m_commandPool.destroy( m_device );
    m_framebuffer.destroy( m_device );
    m_pipeline.destroy( m_device );
    m_renderPass.destroy( m_device );
    m_swapchain.destroy( m_device );
    m_device.destroy();
    m_surface.destroy( m_instance );
    m_instance.destroy();

    glfwDestroyWindow( m_window );
    glfwTerminate();
}
