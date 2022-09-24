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

    void create( std::string name, vk::CommandPoolCreateFlagBits flags );
    vk::CommandBuffer createBuffer( std::string name );
    vk::CommandBuffer beginOneTimeSubmit( std::string name );
    void endOneTimeSubmit( vk::CommandBuffer buff );
    void freeBuffer( vk::CommandBuffer buffer );
    void destroy();

   private:
    vk::CommandPool m_commandPool;
    vk::Device m_device;
};
}  // namespace BR
