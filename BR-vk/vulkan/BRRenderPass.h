#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "BRSwapchain.h"
#include "BRDevice.h"

namespace BR
{
class RenderPass
{
   public:
    RenderPass();
    ~RenderPass();

    void create( Device& device, Swapchain& swapchain );
    void destroy( Device& device );
    VkRenderPass get();

   private:
    VkRenderPass m_renderPass;
};
}  // namespace BR
