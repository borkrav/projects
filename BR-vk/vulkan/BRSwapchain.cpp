#include <BRAppState.h>
#include <BRSwapchain.h>
#include <Util.h>

#include <cassert>

using namespace BR;

Swapchain::Swapchain() : m_swapChain( nullptr )
{
}

Swapchain::~Swapchain()
{
    assert( !m_swapChain && m_swapChainImageViews.empty() );
}

void Swapchain::create(  std::string name, GLFWwindow* window )
{
    auto surfaceFormat = vk::SurfaceFormatKHR(
        vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear );

    m_swapChainFormat = surfaceFormat.format;

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

    int width, height;
    glfwGetFramebufferSize( window, &width, &height );

    m_swapChainExtent = vk::Extent2D( width, height );

    auto physicalDevice = AppState::instance().getPhysicalDevice();
    m_device = AppState::instance().getLogicalDevice();
    auto familyIndex = AppState::instance().getFamilyIndex();
    vk::SurfaceKHR surface = AppState::instance().getSurface();
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR( surface );
    uint32_t imageCount = capabilities.minImageCount + 1;

    /*
    * We ask for a swap chain from the graphics driver
    * We don't own the images or the memory
    * Driver talks to the Win32 API to facilitate the actual
    *   displaying of the image on the screen
    *
    * This is probably all in VRAM, but it's likely driver
    *   dependant
    *
    * VkMemory is a sequence of N bytes
    * VkImage adds information about the format - allows access by
    *   texel, or RGBA block, etc - what's in the memory?
    * VkImageView - similar  to stringView or arrayView - a view
    *   into the memory - selects a part of the underlying VkImage
    * VkFrameBuffer - binds a VkImageView with an attachment
    * VkRenderPass - defines which attachment is drawn into
    */

    auto createInfo = vk::SwapchainCreateInfoKHR();
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = true;
    createInfo.oldSwapchain = nullptr;

    try
    {
        m_swapChain = m_device.createSwapchainKHR( createInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create swap chain!" );
    }

    DEBUG_NAME( m_swapChain, name );

    m_swapChainImages = m_device.getSwapchainImagesKHR( m_swapChain );

    printf( "\nCreated Swap Chain\n" );
    printf( "\tFormat: %s\n", "VK_FORMAT_B8G8R8A8_SRGB" );
    printf( "\tColor Space: %s\n", "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR" );
    printf( "\tPresent Mode: %s\n", "VK_PRESENT_MODE_IMMEDIATE_KHR" );
    printf( "\tExtent: %d %d\n", width, height );
    printf( "\tImage Count: %d\n", imageCount );
}

void Swapchain::createImageViews( std::string name )
{
    /*
    Create image views into the swap chain images
    */

    m_swapChainImageViews.resize( m_swapChainImages.size() );

    for ( size_t i = 0; i < m_swapChainImages.size(); i++ )
    {
        auto createInfo = vk::ImageViewCreateInfo();

        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = m_swapChainFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        ;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        ;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        ;
        createInfo.subresourceRange.aspectMask =
            vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        try
        {
            m_swapChainImageViews[i] = m_device.createImageView( createInfo );

            std::string debugname = name + std::to_string( i );

            DEBUG_NAME( m_swapChainImageViews[i], debugname )
        }
        catch ( vk::SystemError err )
        {
            throw std::runtime_error( "failed to create image views!" );
        }
    }
    printf( "\nCreated %d Image Views: %s\n",
            static_cast<int>( m_swapChainImages.size() ),
            "VK_IMAGE_VIEW_TYPE_2D" );
}

void Swapchain::destroy()
{
    for ( auto imageView : m_swapChainImageViews )
        m_device.destroyImageView( imageView );

    m_device.destroySwapchainKHR( m_swapChain );

    m_swapChain = nullptr;
    m_swapChainImageViews.clear();
}
