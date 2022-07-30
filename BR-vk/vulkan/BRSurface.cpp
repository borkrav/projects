#include <BRSurface.h>
#include <Util.h>
#include <cassert>

using namespace BR;

Surface::Surface()
    : m_surface( VK_NULL_HANDLE )
{
}

Surface::~Surface()
{
    assert( m_surface == VK_NULL_HANDLE );
}

void Surface::create(Instance &instance, GLFWwindow* m_window )
{
     VkResult result =
        glfwCreateWindowSurface( instance.get(), m_window, nullptr, &m_surface );
    checkSuccess( result );

    printf( "\nCreated Surface\n" );
}

void Surface::destroy( Instance& instance )
{
    vkDestroySurfaceKHR( instance.get(), m_surface, nullptr );
    m_surface = VK_NULL_HANDLE;
}

VkSurfaceKHR Surface::get()
{
    assert( m_surface != VK_NULL_HANDLE );
    return m_surface;
}