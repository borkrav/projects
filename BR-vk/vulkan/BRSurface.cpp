#include <BRAppState.h>
#include <BRSurface.h>
#include <BRUtil.h>

#include <cassert>

using namespace BR;

Surface::Surface() : m_surface( VK_NULL_HANDLE )
{
}

Surface::~Surface()
{
    assert( m_surface == VK_NULL_HANDLE );
}

void Surface::create( GLFWwindow* m_window )
{
    VkResult result = glfwCreateWindowSurface(
        AppState::instance().getInstance(), m_window, nullptr, &m_surface );
    CHECK_SUCCESS( result );
}

void Surface::destroy()
{
    vkDestroySurfaceKHR( AppState::instance().getInstance(), m_surface,
                         nullptr );
    m_surface = VK_NULL_HANDLE;
}
