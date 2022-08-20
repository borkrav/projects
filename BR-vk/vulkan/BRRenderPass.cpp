#include <BRAppState.h>
#include <BRRenderPass.h>
#include <BRUtil.h>

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

    std::vector<vk::SubpassDescription> subpasses( 2, subpass );

    vk::SubpassDependency dependency1 = {};
    dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency1.dstSubpass = 0;
    dependency1.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependency1.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency1.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependency1.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                vk::AccessFlagBits::eColorAttachmentWrite;
    dependency1.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::SubpassDependency dependency2 = {};
    dependency2.srcSubpass = 0;
    dependency2.dstSubpass = 1;
    dependency2.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency2.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependency2.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                vk::AccessFlagBits::eColorAttachmentWrite;
    dependency2.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependency2.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    std::vector<vk::SubpassDependency> dependencies{ dependency1, dependency2 };

    /*
    * The dependancy ensures the image is written into only after it's been read from
    * This allows us to run part of the pipeline while the frame is still being presented
    */

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 2;
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

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