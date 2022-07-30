#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "BRDevice.h"

namespace BR
{
class CommandPool
{
   public:
    CommandPool();
    ~CommandPool();

    void create( Device& device );
    void createBuffer( Device& device );
    void destroy( Device& device );

    VkCommandBuffer getBuffer();

   private:
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;
};
}  // namespace BR
