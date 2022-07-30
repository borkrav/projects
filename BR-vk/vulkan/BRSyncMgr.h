#pragma once

#include <vulkan/vulkan_handles.hpp>

#include <BRDevice.h>
#include <vector>

namespace BR
{
class SyncMgr
{
   public:
    SyncMgr();
    ~SyncMgr();

    VkSemaphore createSemaphore( Device& device );
    VkFence createFence( Device& device );
    void destroy( Device& device );

   private:
    std::vector<VkSemaphore> m_semaphores;
    std::vector<VkFence> m_fences;
};
}  // namespace BR
