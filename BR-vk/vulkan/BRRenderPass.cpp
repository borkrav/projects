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
    assert( !m_renderPass && !m_renderPassRT );
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

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = vk::Format::eD32Sfloat;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout =
        vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass1;
    subpass1.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass1.colorAttachmentCount = 1;
    subpass1.pColorAttachments = &colorAttachmentRef;
    subpass1.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDescription subpass2;
    subpass2.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass2.colorAttachmentCount = 1;
    subpass2.pColorAttachments = &colorAttachmentRef;

    std::vector<vk::SubpassDescription> subpasses;
    subpasses.push_back( subpass1 );
    subpasses.push_back( subpass2 );

    vk::SubpassDependency dependency1 = {};
    dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency1.dstSubpass = 0;
    dependency1.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependency1.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency1.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependency1.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    dependency1.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::SubpassDependency dependency2 = {};
    dependency2.srcSubpass = 0;
    dependency2.dstSubpass = 1;
    dependency2.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency2.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependency2.srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    dependency2.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependency2.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    std::vector<vk::SubpassDependency> dependencies{ dependency1, dependency2 };

    /*
    * The dependancy ensures the image is written into only after it's been read from
    * This allows us to run part of the pipeline while the frame is still being presented
    */

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment,
                                                             depthAttachment };

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments.data();
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
}

void RenderPass::createRT( std::string name )
{
    auto format = AppState::instance().getSwapchainFormat();
    m_device = AppState::instance().getLogicalDevice();

    auto colorAttachment = vk::AttachmentDescription();
    colorAttachment.format = format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eGeneral;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = vk::Format::eD32Sfloat;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout =
        vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass1;
    subpass1.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass1.colorAttachmentCount = 1;
    subpass1.pColorAttachments = &colorAttachmentRef;
    subpass1.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDescription subpass2;
    subpass2.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass2.colorAttachmentCount = 1;
    subpass2.pColorAttachments = &colorAttachmentRef;

    std::vector<vk::SubpassDescription> subpasses;
    subpasses.push_back( subpass1 );
    subpasses.push_back( subpass2 );

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

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment,
                                                             depthAttachment };

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 2;
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    try
    {
        m_renderPassRT = m_device.createRenderPass( renderPassInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create render pass!" );
    }

    DEBUG_NAME( m_renderPassRT, name );
}

void RenderPass::destroy()
{
    m_device.destroyRenderPass( m_renderPass );
    m_device.destroyRenderPass( m_renderPassRT );
    m_renderPass = nullptr;
    m_renderPassRT = nullptr;
}

vk::RenderPass RenderPass::get()
{
    assert( m_renderPass );
    return m_renderPass;
}

vk::RenderPass RenderPass::getRT()
{
    assert( m_renderPassRT );
    return m_renderPassRT;
}