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

void RenderPass::addAttachment( vk::Format format, vk::AttachmentLoadOp load,
                                vk::AttachmentStoreOp store,
                                vk::ImageLayout initial, vk::ImageLayout final,
                                vk::ImageLayout ref )
{
    vk::AttachmentDescription attachment;
    attachment.format = format;
    attachment.samples = vk::SampleCountFlagBits::e1;
    attachment.loadOp = load;
    attachment.storeOp = store;
    attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment.initialLayout = initial;
    attachment.finalLayout = final;

    vk::AttachmentReference attachmentRef;
    attachmentRef.attachment = m_attachments.size();
    attachmentRef.layout = ref;

    m_attachments.push_back( attachment );
    m_attachmentRefs.push_back( attachmentRef );

    /*
    before subpass begins
        attachment will be in *initial* layout
    during subpass
        attachment will be in *ref* layout
    after subpass ends
        attachment will be in *final* layout
    */
}

void RenderPass::addSubpass( vk::PipelineBindPoint bind, int color, int depth )
{
    vk::SubpassDescription sub;
    sub.pipelineBindPoint = bind;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &m_attachmentRefs[color];
    sub.pDepthStencilAttachment = &m_attachmentRefs[depth];

    m_subpasses.push_back( sub );
}

void RenderPass::addDependency( uint32_t src, uint32_t dest,
                                vk::PipelineStageFlags srcStageMask,
                                vk::PipelineStageFlags dstStageMask,
                                vk::AccessFlags srcAccessMask,
                                vk::AccessFlags dstAccessMask )
{
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependency.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                               vk::AccessFlagBits::eColorAttachmentWrite |
                               vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    m_dependencies.push_back( dependency );
}

void RenderPass::build( std::string name )
{
    assert( !m_renderPass );

    m_device = AppState::instance().getLogicalDevice();

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = m_attachments.size();
    renderPassInfo.pAttachments = m_attachments.data();
    renderPassInfo.subpassCount = m_subpasses.size();
    renderPassInfo.pSubpasses = m_subpasses.data();
    renderPassInfo.dependencyCount = m_dependencies.size();
    renderPassInfo.pDependencies = m_dependencies.data();

    try
    {
        m_renderPass = m_device.createRenderPass( renderPassInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create render pass!" );
    }

    DEBUG_NAME( m_renderPass, name );
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