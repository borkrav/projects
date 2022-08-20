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

void Framebuffer::create(std::string name, RenderPass& renderpass )
{
    /*
    * The framebuffer binds the VkImageViews from the swap chain
    *   with the attachement that we specified in the render pass
    */

    auto imageViews = AppState::instance().getImageViews();
    auto extent = AppState::instance().getSwapchainExtent();
    m_device = AppState::instance().getLogicalDevice();

    m_swapChainFramebuffers.resize( imageViews.size() );

    for ( size_t i = 0; i < imageViews.size(); i++ )
    {
        vk::ImageView attachments[] = { imageViews[i] };

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = renderpass.get();
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        try
        {
            m_swapChainFramebuffers[i] =
                m_device.createFramebuffer( framebufferInfo );

            std::string namestring = name + std::to_string( i );

            DEBUG_NAME( m_swapChainFramebuffers[i], namestring );
        }
        catch ( vk::SystemError err )
        {
            throw std::runtime_error( "failed to create framebuffer!" );
        }
    }

    printf( "\nCreated %d Frame Buffers\n",
            static_cast<int>( imageViews.size() ) );
}

void Framebuffer::destroy()
{
    for ( auto framebuffer : m_swapChainFramebuffers )
        m_device.destroyFramebuffer( framebuffer );

    m_swapChainFramebuffers.clear();
}

std::vector<vk::Framebuffer>& Framebuffer::get()
{
    return m_swapChainFramebuffers;
}