#include <BRRenderPass.h>
#include <Util.h>
#include <cassert>

using namespace BR;

RenderPass::RenderPass()
    : m_renderPass( VK_NULL_HANDLE )
{
}

RenderPass::~RenderPass()
{
    assert( m_renderPass == VK_NULL_HANDLE );
}

void RenderPass::create( Device& device, Swapchain& swapchain )
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

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain.getFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    /*
    * The dependancy ensures the image is written into only after it's been read from
    * This allows us to run part of the pipeline while the frame is still being presented
    */

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(
        device.getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass );

    checkSuccess( result );

    printf( "\nCreated Render Pass\n" );
}

void RenderPass::destroy( Device& device )
{
    vkDestroyRenderPass( device.getLogicalDevice(), m_renderPass, nullptr );
    
    m_renderPass = VK_NULL_HANDLE;
}

VkRenderPass RenderPass::get()
{
    assert( m_renderPass != VK_NULL_HANDLE );
    return m_renderPass;
}