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
    m_device.destroy();
    m_surface.destroy();
    m_instance.destroy();
}

void AppState::init( GLFWwindow* window )
{
    if ( created )
        return;

    created = true;

    m_window = window;

    m_instance.create( true );
    m_surface.create( window );

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    m_device.create( deviceExtensions );
    m_swapchain.create( window, m_surface );
    m_swapchain.createImageViews();
}

void AppState::recreateSwapchain()
{
    //destroy old swapchain
    m_swapchain.destroy();

    //create new swapchain
    m_swapchain.create( m_window, m_surface );
    m_swapchain.createImageViews();
}

VkInstance AppState::getInstance()
{
    assert( created );
    return m_instance.get();
}

VkPhysicalDevice AppState::getPhysicalDevice()
{
    assert( created );
    return m_device.getPhysicaDevice();
}

VkDevice AppState::getLogicalDevice()
{
    assert( created );
    return m_device.getLogicalDevice();
}

VkQueue AppState::getGraphicsQueue()
{
    assert( created );
    return m_device.getGraphicsQueue();
}

int AppState::getFamilyIndex()
{
    assert( created );
    return m_device.getFamilyIndex();
}

VkSurfaceKHR AppState::getSurface()
{
    assert( created );
    return m_surface.get();
}

VkSwapchainKHR AppState::getSwapchain()
{
    assert( created );
    return m_swapchain.get();
}

VkFormat AppState::getSwapchainFormat()
{
    assert( created );
    return m_swapchain.getFormat();
}

VkExtent2D& AppState::getSwapchainExtent()
{
    assert( created );
    return m_swapchain.getExtent();
}

std::vector<VkImageView>& AppState::getImageViews()
{
    assert( created );
    return m_swapchain.getImageViews();
}