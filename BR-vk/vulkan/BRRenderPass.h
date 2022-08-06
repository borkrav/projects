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

    void create( std::string name );
    void destroy();
    vk::RenderPass get();

   private:
    vk::RenderPass m_renderPass;
    vk::Device m_device;
};
}  // namespace BR
