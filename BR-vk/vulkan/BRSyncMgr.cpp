#include "BRSyncMgr.h"

#include <cassert>

#include "Util.h"

using namespace BR;

SyncMgr::SyncMgr()
{
}

SyncMgr::~SyncMgr()
{
    assert( m_fences.empty() && m_semaphores.empty() );
}

VkSemaphore SyncMgr::createSemaphore( Device& device )
{
    VkSemaphore sem;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result = vkCreateSemaphore( device.getLogicalDevice(),
                                         &semaphoreInfo, nullptr, &sem );

    checkSuccess( result );

    return sem;
}

VkFence SyncMgr::createFence( Device& device )
{
    VkFence fence;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateFence( device.getLogicalDevice(), &fenceInfo, nullptr,
                            &fence );
    checkSuccess( result );

    return fence;
}

void SyncMgr::destroy( Device& device )
{
    for ( auto sem : m_semaphores )
        vkDestroySemaphore( device.getLogicalDevice(), sem, nullptr );

    for ( auto fence : m_fences )
        vkDestroyFence( device.getLogicalDevice(), fence, nullptr );

    m_semaphores.clear();
    m_fences.clear();
}
