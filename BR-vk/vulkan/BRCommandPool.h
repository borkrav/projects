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
    VkCommandBuffer createBuffer();
    void destroy();

   private:
    VkCommandPool m_commandPool;
};
}  // namespace BR
