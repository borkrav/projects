#include <BRAppState.h>
#include <BRSyncMgr.h>
#include <BRUtil.h>

#include <cassert>

using namespace BR;

SyncMgr::SyncMgr() : m_device( nullptr )
{
}

SyncMgr::~SyncMgr()
{
    assert( m_fences.empty() && m_semaphores.empty() );
}

vk::Semaphore SyncMgr::createSemaphore( std::string name )
{
    if ( !m_device )
        m_device = AppState::instance().getLogicalDevice();

    auto info = vk::SemaphoreCreateInfo();

    try
    {
        vk::Semaphore sem = m_device.createSemaphore( info );
        m_semaphores.push_back( sem );
        DEBUG_NAME( sem, name );
        return sem;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error(
            "failed to create synchronization objects for a frame!" );
    }
}

vk::Fence SyncMgr::createFence( std::string name )
{
    if ( !m_device )
        m_device = AppState::instance().getLogicalDevice();

    auto info = vk::FenceCreateInfo( vk::FenceCreateFlagBits::eSignaled );

    try
    {
        vk::Fence fence = m_device.createFence( info );
        m_fences.push_back( fence );
        DEBUG_NAME( fence, name );
        return fence;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error(
            "failed to create synchronization objects for a frame!" );
    }
}

void SyncMgr::destroy()
{
    for ( auto sem : m_semaphores )
        m_device.destroySemaphore( sem );

    for ( auto fence : m_fences )
        m_device.destroyFence( fence );

    m_semaphores.clear();
    m_fences.clear();
}
