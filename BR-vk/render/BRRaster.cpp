#include "BRRaster.h"

#include <ranges>

#include "BRAppState.h"
#include "BRRender.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

using namespace BR;

Raster::Raster()
    : m_descMgr( AppState::instance().getDescMgr() ),
      m_bufferAlloc( AppState::instance().getMemoryMgr() ),
      m_framesInFlight( AppState::instance().m_framesInFlight )
{
    m_device = AppState::instance().getLogicalDevice();
}

void Raster::init()
{
    m_descriptorSetLayout = m_descMgr.createLayout(
        "UBO layout", std::vector<BR::DescMgr::Binding>{
                          { 0, vk::DescriptorType::eUniformBuffer, 1,
                            vk::ShaderStageFlagBits::eVertex } } );

    createDepthBuffer();
    createRenderPass();

    m_framebuffer.create( "Swapchain Frame buffer", m_renderPass,
                          m_depthBufferView );

    createPipeline();
}

void Raster::createPipeline()
{
    auto swapChainExtent = AppState::instance().getSwapchainExtent();

    m_pipeline.addShaderStage( "build/shaders/shader.vert.spv",
                               vk::ShaderStageFlagBits::eVertex );
    m_pipeline.addShaderStage( "build/shaders/shader.frag.spv",
                               vk::ShaderStageFlagBits::eFragment );

    auto bindingDescription = Scene::Vertex::getBindingDescription();
    auto attributeDescriptions = Scene::Vertex::getAttributeDescriptions();

    m_pipeline.addVertexInputInfo( bindingDescription, attributeDescriptions );
    m_pipeline.addInputAssembly( vk::PrimitiveTopology::eTriangleList,
                                 VK_FALSE );
    m_pipeline.addViewport( swapChainExtent );
    m_pipeline.addRasterizer( vk::CullModeFlagBits::eBack,
                              vk::FrontFace::eCounterClockwise );
    m_pipeline.addDepthSencil( vk::CompareOp::eLess );
    m_pipeline.addMultisampling( vk::SampleCountFlagBits::e1 );
    m_pipeline.addColorBlend();
    m_pipeline.addDynamicStates(
        { vk::DynamicState::eScissor, vk::DynamicState::eViewport } );
    m_pipeline.build( "Raster Pipeline", m_renderPass, m_descriptorSetLayout );
}

void Raster::createRenderPass()
{
    auto format = AppState::instance().getSwapchainFormat();

    m_renderPass.addAttachment(
        format, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
        vk::ImageLayout::eColorAttachmentOptimal );

    m_renderPass.addAttachment(
        vk::Format::eD32Sfloat, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::ImageLayout::eDepthStencilAttachmentOptimal );

    m_renderPass.addSubpass( vk::PipelineBindPoint::eGraphics, 0, 1 );

    m_renderPass.addDependency(
        VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::AccessFlagBits::eMemoryRead,
        vk::AccessFlagBits::eColorAttachmentRead |
            vk::AccessFlagBits::eColorAttachmentWrite |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite );

    m_renderPass.build( "Raster Renderpass" );
}

void Raster::createDepthBuffer()
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

void Raster::createDescriptorSets( std::vector<vk::Buffer>& uniforms,
                                   vk::DescriptorPool pool )
{
    m_descriptorSets.push_back( m_descMgr.createSet(
        "Frame 1 Desc set", m_descriptorSetLayout, pool ) );
    m_descriptorSets.push_back( m_descMgr.createSet(
        "Frame 2 Desc set", m_descriptorSetLayout, pool ) );

    for ( int i : std::views::iota( 0, m_framesInFlight ) )
    {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = uniforms[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( BRRender::UniformBufferObject );

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

void Raster::recordDrawCommandBuffer( vk::CommandBuffer commandBuffer,
                                      uint32_t imageIndex, int currentFrame,
                                      vk::Buffer vertexBuffer,
                                      vk::Buffer indexBuffer, int drawCount )
{
    /*
    * Do a render pass
    * Give it the framebuffer index -> the attachement we're drawing into   
    * Bind the graphics pipeline
    * Draw call
    */

    auto extent = AppState::instance().getSwapchainExtent();

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

    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };

    commandBuffer.bindVertexBuffers( 0, 1, vertexBuffers, offsets );
    commandBuffer.bindIndexBuffer( indexBuffer, 0, vk::IndexType::eUint32 );

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getLayout(),
        0, 1, (VkDescriptorSet*)&m_descriptorSets[currentFrame], 0, nullptr );

    commandBuffer.drawIndexed( static_cast<uint32_t>( drawCount ), 1, 0, 0, 0 );

    commandBuffer.endRenderPass();
}

void Raster::resize()
{
    m_bufferAlloc.free( m_depthBuffer );
    m_framebuffer.destroy();
    createDepthBuffer();

    m_framebuffer.create( "Swapchain Frame buffer", m_renderPass,
                          m_depthBufferView );
}

Framebuffer& Raster::getFrameBuffer()
{
    return m_framebuffer;
}

void Raster::destroy()
{
    m_framebuffer.destroy();
    m_pipeline.destroy();
    m_renderPass.destroy();
}