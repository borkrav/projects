#include <BRAppState.h>
#include <BRFramebuffer.h>
#include <Util.h>

#include <cassert>

using namespace BR;

Framebuffer::Framebuffer()
{
}

Framebuffer::~Framebuffer()
{
    assert( m_swapChainFramebuffers.empty() );
}

void Framebuffer::create( RenderPass& renderpass )
{
    /*
    * The framebuffer binds the VkImageViews from the swap chain
    *   with the attachement that we specified in the render pass
    */

    auto imageViews = AppState::instance().getImageViews();
    auto extent = AppState::instance().getSwapchainExtent();

    m_swapChainFramebuffers.resize( imageViews.size() );

    for ( size_t i = 0; i < imageViews.size(); i++ )
    {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderpass.get();
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(
            AppState::instance().getLogicalDevice(), &framebufferInfo, nullptr,
            &m_swapChainFramebuffers[i] );
        checkSuccess( result );
    }

    printf( "\nCreated %d Frame Buffers\n",
            static_cast<int>( imageViews.size() ) );
}

void Framebuffer::destroy()
{
    for ( auto framebuffer : m_swapChainFramebuffers )
        vkDestroyFramebuffer( AppState::instance().getLogicalDevice(),
                              framebuffer, nullptr );

    m_swapChainFramebuffers.clear();
}

std::vector<VkFramebuffer>& Framebuffer::get()
{
    return m_swapChainFramebuffers;
}