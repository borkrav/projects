#include <BRAppState.h>
#include <BRSyncMgr.h>
#include <Util.h>

#include <cassert>

using namespace BR;

SyncMgr::SyncMgr()
{
}

SyncMgr::~SyncMgr()
{
    assert( m_fences.empty() && m_semaphores.empty() );
}

VkSemaphore SyncMgr::createSemaphore()
{
    VkSemaphore sem;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result =
        vkCreateSemaphore( AppState::instance().getLogicalDevice(),
                           &semaphoreInfo, nullptr, &sem );

    checkSuccess( result );

    return sem;
}

VkFence SyncMgr::createFence()
{
    VkFence fence;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateFence( AppState::instance().getLogicalDevice(),
                                     &fenceInfo, nullptr, &fence );
    checkSuccess( result );

    return fence;
}

void SyncMgr::destroy()
{
    for ( auto sem : m_semaphores )
        vkDestroySemaphore( AppState::instance().getLogicalDevice(), sem,
                            nullptr );

    for ( auto fence : m_fences )
        vkDestroyFence( AppState::instance().getLogicalDevice(), fence,
                        nullptr );

    m_semaphores.clear();
    m_fences.clear();
}
