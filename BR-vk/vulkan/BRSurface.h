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

    void create( GLFWwindow* m_window );
    void destroy();

    VkSurfaceKHR m_surface;
};
}  // namespace BR
