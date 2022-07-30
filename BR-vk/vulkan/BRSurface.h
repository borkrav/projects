#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <BRInstance.h>

namespace BR
{
class Surface
{
   public:
    Surface();
    ~Surface();

    void create( Instance &instance, GLFWwindow* m_window);
    void destroy(Instance &instance);
    VkSurfaceKHR get();

   private:
    VkSurfaceKHR m_surface;
};
}  // namespace BR
