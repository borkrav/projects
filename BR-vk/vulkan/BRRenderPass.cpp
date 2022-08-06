#include <BRAppState.h>
#include <BRRenderPass.h>
#include <Util.h>

#include <cassert>

using namespace BR;

RenderPass::RenderPass() : m_renderPass( VK_NULL_HANDLE )
{
}

RenderPass::~RenderPass()
{
    assert( !m_renderPass );
}

void RenderPass::create( std::string name )
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

    auto format = AppState::instance().getSwapchainFormat();
    m_device = AppState::instance().getLogicalDevice();

    auto colorAttachment = vk::AttachmentDescription();
    colorAttachment.format = format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    //dependency.srcAccessMask = 0;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                               vk::AccessFlagBits::eColorAttachmentWrite;

    /*
    * The dependancy ensures the image is written into only after it's been read from
    * This allows us to run part of the pipeline while the frame is still being presented
    */

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    try
    {
        m_renderPass = m_device.createRenderPass( renderPassInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create render pass!" );
    }

    DEBUG_NAME( m_renderPass, name );

    printf( "\nCreated Render Pass\n" );
}

void RenderPass::destroy()
{
    m_device.destroyRenderPass( m_renderPass );
    m_renderPass = nullptr;
}

vk::RenderPass RenderPass::get()
{
    assert( m_renderPass );
    return m_renderPass;
}