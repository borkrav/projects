#pragma once

#include <BRDevice.h>
#include <BRSwapchain.h>

#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class RenderPass
{
   public:
    RenderPass();
    ~RenderPass();

    void create();
    void destroy();
    VkRenderPass get();

   private:
    VkRenderPass m_renderPass;
};
}  // namespace BR
