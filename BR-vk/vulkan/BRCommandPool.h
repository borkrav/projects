#pragma once

#include <BRDevice.h>

#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class CommandPool
{
   public:
    CommandPool();
    ~CommandPool();

    void create();
    vk::CommandBuffer createBuffer();
    void destroy();

   private:
    vk::CommandPool m_commandPool;
    vk::Device m_device;
};
}  // namespace BR
