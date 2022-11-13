#include <BRAppState.h>
#include <BRSwapchain.h>
#include <BRUtil.h>

#include <lodepng.h>
#include <chrono>

#include <cassert>

using namespace BR;

Swapchain::Swapchain() : m_swapChain( nullptr )
{
}

Swapchain::~Swapchain()
{
    assert( !m_swapChain && m_swapChainImageViews.empty() );
}

void Swapchain::create( std::string name, GLFWwindow* window,
                        vk::PhysicalDevice physicalDevice,
                        vk::Device logicalDevice, int familyIndex,
                        vk::SurfaceKHR surface )
{
    auto surfaceFormat = vk::SurfaceFormatKHR(
        vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear );

    m_swapChainFormat = surfaceFormat.format;

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

    int width, height;
    glfwGetFramebufferSize( window, &width, &height );

    m_swapChainExtent = vk::Extent2D( width, height );

    m_device = logicalDevice;
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
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                            vk::ImageUsageFlagBits::eTransferSrc |
                            vk::ImageUsageFlagBits::eStorage;
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

    //DEBUG_NAME( m_swapChain, name );

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
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
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

            //DEBUG_NAME( m_swapChainImageViews[i], debugname )
        }
        catch ( vk::SystemError err )
        {
            throw std::runtime_error( "failed to create image views!" );
        }
    }
}

void Swapchain::takeScreenshot( CommandPool& pool, int frame )
{
    printf( "taking screenshot!\n" );

    auto extent = AppState::instance().getSwapchainExtent();
    auto format = AppState::instance().getSwapchainFormat();

    auto& bufferAlloc = AppState::instance().getMemoryMgr();

    vk::Image srcImage =
        m_swapChainImages[frame];

    // Create Destination image for the screenshot, linear tiling (regular array, so I can copy out)
    // Layouts is how the image is stored in VRAM, need to select the optimal layout for the operation
    // Layouts are probably related to internal compression, where the hardware compresses the image to save
    //      bandwidth - need to explicitely indicate that we plan to do operation X to the image, so it can be
    //      properly accessed
    // Basically, we tell the driver what we're planning to do with the image, and the driver ensures that
    //      those operations are optimal, by changing the layout. What this means is up to the driver/GPU

    vk::Image dstImage = bufferAlloc.createImage(
        "Screenshot Destination Image", extent.width, extent.height,
        vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eLinear,
        vk::ImageUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent );

    vk::DeviceMemory dstMem = bufferAlloc.getMemory( dstImage );

    auto cmdBuff =
        pool.beginOneTimeSubmit( "Screenshot Command Buffer" );

    //asking for color
    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    // none -> transfer write
    // undefined -> optimal transfer destination
    imageBarrier( cmdBuff, dstImage, range, vk::AccessFlagBits::eNone,
                  vk::AccessFlagBits::eTransferWrite,
                  vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eTransferDstOptimal );

    // memory read -> transfer read
    // presentation -> transfer source
    imageBarrier( cmdBuff, srcImage, range, vk::AccessFlagBits::eMemoryRead,
                  vk::AccessFlagBits::eTransferRead,
                  vk::ImageLayout::ePresentSrcKHR,
                  vk::ImageLayout::eTransferSrcOptimal );

    //copy the color, width*height data
    vk::ImageCopy imageCopy;
    imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopy.srcSubresource.layerCount = 1;
    imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopy.dstSubresource.layerCount = 1;
    imageCopy.extent.width = extent.width;
    imageCopy.extent.height = extent.height;
    imageCopy.extent.depth = 1;

    //do the actual copy
    cmdBuff.copyImage( srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage,
                       vk::ImageLayout::eTransferDstOptimal, imageCopy );

    // transfer write -> memory read
    // transfer destination -> general
    imageBarrier( cmdBuff, dstImage, range, vk::AccessFlagBits::eTransferWrite,
                  vk::AccessFlagBits::eMemoryRead,
                  vk::ImageLayout::eTransferDstOptimal,
                  vk::ImageLayout::eGeneral );

    // transfer read -> memory read
    // transfer source -> presentation
    imageBarrier( cmdBuff, srcImage, range, vk::AccessFlagBits::eTransferRead,
                  vk::AccessFlagBits::eMemoryRead,
                  vk::ImageLayout::eTransferSrcOptimal,
                  vk::ImageLayout::ePresentSrcKHR );

    pool.endOneTimeSubmit( cmdBuff );

    //get layout
    vk::ImageSubresource subResource( vk::ImageAspectFlagBits::eColor, 0, 0 );
    auto subResourceLayout =
        m_device.getImageSubresourceLayout( dstImage, subResource );

    //map image to host memory
    const char* data =
        (const char*)m_device.mapMemory( dstMem, 0, VK_WHOLE_SIZE );

    data += subResourceLayout.offset;

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface formats for demonstation purposes

    std::vector<vk::Format> formatsBGR = { vk::Format::eB8G8R8A8Srgb,
                                           vk::Format::eB8G8R8A8Unorm,
                                           vk::Format::eB8G8R8A8Snorm };

    colorSwizzle = ( std::find( formatsBGR.begin(), formatsBGR.end(),
                                format ) != formatsBGR.end() );

    //fill the image vector
    std::vector<unsigned char> image;

    uint64_t w = extent.width;
    uint64_t h = extent.height;

    image.resize( w * h * 4 );

    for ( uint64_t y = 0; y < h; y++ )
    {
        //go through data byte by byte
        unsigned char* row = (unsigned char*)data;
        for ( uint64_t x = 0; x < w; x++ )
        {
            if ( colorSwizzle )
            {
                image[4 * w * y + 4 * x + 0] = *( row + 2 );
                image[4 * w * y + 4 * x + 1] = *( row + 1 );
                image[4 * w * y + 4 * x + 2] = *( row + 0 );
                image[4 * w * y + 4 * x + 3] = 255;
            }

            else
            {
                image[4 * w * y + 4 * x + 0] = *( row + 0 );
                image[4 * w * y + 4 * x + 1] = *( row + 1 );
                image[4 * w * y + 4 * x + 2] = *( row + 2 );
                image[4 * w * y + 4 * x + 3] = 255;
            }

            //increment by 4 bytes (RGBA)
            row += 4;
        }
        //go to next row
        data += subResourceLayout.rowPitch;
    }

    //create filename, with current date and time
    auto p = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t( p );
    char str[26];
    ctime_s( str, sizeof str, &t );
    std::string fileName = "screenshots/" + (std::string)str + ".png";

    //clean up the string
    fileName.erase( std::remove( fileName.begin(), fileName.end(), '\n' ),
                    fileName.end() );
    std::replace( fileName.begin(), fileName.end(), ' ', '-' );
    std::replace( fileName.begin(), fileName.end(), ':', '-' );

    //encode to PNG
    unsigned error = lodepng::encode( fileName, image.data(), w, h );

    assert( error == 0 );

    m_device.unmapMemory( dstMem );
    bufferAlloc.free( dstImage );
}

void Swapchain::destroy()
{
    for ( auto imageView : m_swapChainImageViews )
        m_device.destroyImageView( imageView );

    m_device.destroySwapchainKHR( m_swapChain );

    m_swapChain = nullptr;
    m_swapChainImageViews.clear();
}
