#include <BRRender.h>
#include <BRUtil.h>

#include <algorithm>
#include <cassert>
#include <ranges>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <lodepng.h>

#define GLM_FORCE_RADIANS
#include <chrono>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

void BRRender::loadModel()
{
    tinyobj::ObjReader reader;  // Used to read an OBJ file
    reader.ParseFromFile( "models/cube.obj" );

    assert( reader.Valid() );  // Make sure tinyobj was able to parse this file
    const std::vector<tinyobj::real_t> objVertices =
        reader.GetAttrib().GetVertices();
    const std::vector<tinyobj::shape_t>& objShapes =
        reader.GetShapes();  // All shapes in the file
    assert( objShapes.size() == 1 );

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

    printf( "Loaded file!\n" );
}

void BRRender::initVulkan()
{
    AppState::instance().init( m_window, enableValidationLayers );

    m_device = AppState::instance().getLogicalDevice();

    m_renderPass.create( "Raster Renderpass" );

    m_descriptorSetLayout = m_descMgr.createLayout(
        "UBO layout", std::vector<BR::DescMgr::Binding>{
                          { 0, vk::DescriptorType::eUniformBuffer, 1,
                            vk::ShaderStageFlagBits::eVertex } } );

    m_descriptorPool = m_descMgr.createPool(
        "UBO pool", MAX_FRAMES_IN_FLIGHT,
        std::vector<BR::DescMgr::PoolSize>{
            { vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT } } );

    m_pipeline.create( "Raster Pipeline", m_renderPass, m_descriptorSetLayout );
    m_framebuffer.create( "Swapchain Frame buffer", m_renderPass );
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

    loadModel();

    m_vertexBuffer = m_bufferAlloc.createAndStageBuffer(
        "Vertex", m_vertices, vk::BufferUsageFlagBits::eVertexBuffer );
    m_indexBuffer = m_bufferAlloc.createAndStageBuffer(
        "Index", m_indices, vk::BufferUsageFlagBits::eIndexBuffer );

    createDescriptorSets();
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

        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = vk::StructureType::eWriteDescriptorSet;
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

    m_framebuffer.destroy();
    AppState::instance().recreateSwapchain();
    m_framebuffer.create( "Swapchain Frame buffer", m_renderPass );
}

void BRRender::updateUniformBuffer( uint32_t currentImage )
{
    auto extent = AppState::instance().getSwapchainExtent();

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime )
                     .count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ),
                             glm::vec3( 0.0f, 0.0f, 1.0f ) );

    ubo.view = glm::lookAt( glm::vec3( 2.0f, 2.0f, 2.0f ),
                            glm::vec3( 0.0f, 0.0f, 0.0f ),
                            glm::vec3( 0.0f, 0.0f, 1.0f ) );

    ubo.proj =
        glm::perspective( glm::radians( 45.0f ),
                          extent.width / (float)extent.height, 0.1f, 10.0f );

    ubo.proj[1][1] *= -1;

    auto mem = m_bufferAlloc.getMemory( m_uniformBuffers[currentImage] );

    void* data = m_device.mapMemory( mem, 0, sizeof( ubo ) );
    memcpy( data, &ubo, sizeof( ubo ) );
    m_device.unmapMemory( mem );
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
    commandBuffer.bindIndexBuffer( m_indexBuffer, 0, vk::IndexType::eUint16 );

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getLayout(),
        0, 1, (VkDescriptorSet*)&m_descriptorSets[m_currentFrame], 0, nullptr );

    commandBuffer.drawIndexed( static_cast<uint32_t>( m_indices.size() ), 1, 0,
                               0, 0 );
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

    updateUniformBuffer( m_currentFrame );

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
    m_bufferAlloc.destroy();
    m_syncMgr.destroy();
    m_commandPool.destroy();
    m_framebuffer.destroy();
    m_pipeline.destroy();
    m_renderPass.destroy();
    m_descMgr.destroy();

    glfwDestroyWindow( m_window );
    glfwTerminate();
}
