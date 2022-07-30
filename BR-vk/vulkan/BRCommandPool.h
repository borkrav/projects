#pragma once

#include <vulkan/vulkan_handles.hpp>

#include <BRDevice.h>

namespace BR
{
class CommandPool
{
   public:
    CommandPool();
    ~CommandPool();

    void create( Device& device );
    VkCommandBuffer createBuffer( Device& device );
    void destroy( Device& device );

   private:
    VkCommandPool m_commandPool;
};
}  // namespace BR
