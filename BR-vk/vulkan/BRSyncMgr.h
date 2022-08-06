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

    VkSemaphore createSemaphore();
    VkFence createFence();
    void destroy();

   private:
    std::vector<VkSemaphore> m_semaphores;
    std::vector<VkFence> m_fences;
};
}  // namespace BR
