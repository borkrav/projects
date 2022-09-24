#pragma once

#include <BRDevice.h>

#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class SyncMgr
{
   public:
    SyncMgr();
    ~SyncMgr();

    vk::Semaphore createSemaphore( std::string name );
    vk::Fence createFence( std::string name );
    void destroy();

   private:
    std::vector<vk::Semaphore> m_semaphores;
    std::vector<vk::Fence> m_fences;
    vk::Device m_device;
};
}  // namespace BR
