#include <BRSwapchain.h>
#include <Util.h>
#include <cassert>

using namespace BR;

Swapchain::Swapchain() : m_swapChain( VK_NULL_HANDLE )
{
}

Swapchain::~Swapchain()
{
    assert( m_swapChain == VK_NULL_HANDLE && m_swapChainImageViews.empty() );
}

void Swapchain::create( GLFWwindow* window, Device& device, Surface& surface )
{
    VkSurfaceFormatKHR surfaceFormat{};
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    m_swapChainFormat = surfaceFormat.format;

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    int width, height;
    glfwGetFramebufferSize( window, &width, &height );

    m_swapChainExtent = { static_cast<uint32_t>( width ),
                          static_cast<uint32_t>( height ) };

    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device.getPhysicaDevice(),
                                               surface.get(), &capabilities );
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

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface.get();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(
        device.getLogicalDevice(), &createInfo, nullptr, &m_swapChain );
    checkSuccess( result );

    vkGetSwapchainImagesKHR( device.getLogicalDevice(), m_swapChain,
                             &imageCount, nullptr );
    m_swapChainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( device.getLogicalDevice(), m_swapChain,
                             &imageCount, m_swapChainImages.data() );

    printf( "\nCreated Swap Chain\n" );
    printf( "\tFormat: %s\n", "VK_FORMAT_B8G8R8A8_SRGB" );
    printf( "\tColor Space: %s\n", "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR" );
    printf( "\tPresent Mode: %s\n", "VK_PRESENT_MODE_IMMEDIATE_KHR" );
    printf( "\tExtent: %d %d\n", width, height );
    printf( "\tImage Count: %d\n", imageCount );
}

void Swapchain::createImageViews(Device& device)
{
    /*
    Create image views into the swap chain images
    */

    m_swapChainImageViews.resize( m_swapChainImages.size() );

    for ( size_t i = 0; i < m_swapChainImages.size(); i++ )
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result =
            vkCreateImageView( device.getLogicalDevice(), &createInfo,
                               nullptr, &m_swapChainImageViews[i] );
        checkSuccess( result );
    }
    printf( "\nCreated %d Image Views: %s\n",
            static_cast<int>( m_swapChainImages.size() ),
            "VK_IMAGE_VIEW_TYPE_2D" );
}


VkSwapchainKHR Swapchain::get()
{
    assert( m_swapChain != VK_NULL_HANDLE );
    return m_swapChain;
}

VkFormat Swapchain::getFormat()
{
    return m_swapChainFormat;
}

VkExtent2D& Swapchain::getExtent()
{
    return m_swapChainExtent;
}

std::vector<VkImageView>& Swapchain::getImageViews()
{
    return m_swapChainImageViews;
}

void Swapchain::destroy( Device& device )
{
    for ( auto imageView : m_swapChainImageViews )
        vkDestroyImageView( device.getLogicalDevice(), imageView, nullptr );

    vkDestroySwapchainKHR( device.getLogicalDevice(), m_swapChain, nullptr );

    m_swapChain = VK_NULL_HANDLE;
    m_swapChainImageViews.clear();
}

