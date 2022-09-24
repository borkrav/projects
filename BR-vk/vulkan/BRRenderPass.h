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
    void createRT( std::string name );
    void destroy();
    vk::RenderPass get();
    vk::RenderPass getRT();

   private:
    vk::RenderPass m_renderPass;
    vk::RenderPass m_renderPassRT;
    vk::Device m_device;
};
}  // namespace BR
