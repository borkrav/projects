#pragma once

#include <BRInstance.h>

#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Surface
{
    friend class AppState;

   private:
    Surface();
    ~Surface();
    Surface( const Surface& ) = delete;

    void create( GLFWwindow* m_window, vk::Instance instance );
    void destroy();

    VkSurfaceKHR m_surface;
};
}  // namespace BR
