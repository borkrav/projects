#include <BRAppState.h>

#include <cassert>

using namespace BR;

AppState::AppState() : created( false )
{
}

AppState::~AppState()
{
    assert( created );

    m_swapchain.destroy();
    m_surface.destroy();
    m_instance.destroy();
}

void AppState::init( GLFWwindow* window, bool debug )
{
    if ( created )
        return;

    created = true;

    m_window = window;

    m_instance.create( debug );
    m_surface.create( window );

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    m_device.create( "GPU", deviceExtensions );

#ifdef NDEBUG
#else
    m_debug.create( " RTX 3080 " );
#endif

    m_swapchain.create( "Swapchain", window );
    m_swapchain.createImageViews( "Swapchain image view " );
}

void AppState::recreateSwapchain()
{
    //destroy old swapchain
    m_swapchain.destroy();

    //create new swapchain
    m_swapchain.create( "Swapchain", m_window );
    m_swapchain.createImageViews( "Swapchain image view " );
}

vk::Instance AppState::getInstance()
{
    assert( created );
    return m_instance.m_instance.get();
}

vk::PhysicalDevice AppState::getPhysicalDevice()
{
    assert( created );
    return m_device.m_physicalDevice;
}

vk::Device AppState::getLogicalDevice()
{
    assert( created );
    return m_device.m_logicalDevice.get();
}

vk::Queue AppState::getGraphicsQueue()
{
    assert( created );
    return m_device.m_graphicsQueue;
}

int AppState::getFamilyIndex()
{
    assert( created );
    return m_device.m_index;
}

vk::SurfaceKHR AppState::getSurface()
{
    assert( created );
    return m_surface.m_surface;
}

vk::SwapchainKHR AppState::getSwapchain()
{
    assert( created );
    return m_swapchain.m_swapChain;
}

vk::Format AppState::getSwapchainFormat()
{
    assert( created );
    return m_swapchain.m_swapChainFormat;
}

vk::Extent2D& AppState::getSwapchainExtent()
{
    assert( created );
    return m_swapchain.m_swapChainExtent;
}

std::vector<vk::ImageView>& AppState::getImageViews()
{
    assert( created );
    return m_swapchain.m_swapChainImageViews;
}

#ifdef NDEBUG
#else
BR::Debug AppState::getDebug()
{
    return m_debug;
}
#endif