#include <BRAppState.h>
#include <BRCommandPool.h>
#include <Util.h>

#include <cassert>

using namespace BR;

CommandPool::CommandPool() : m_commandPool( nullptr )
{
}

CommandPool::~CommandPool()
{
    assert( !m_commandPool );
}

void CommandPool::create()
{
    /*
    * Command pools allow concurrent recording of command buffers
    * One pool per thread -> Multi-thread recording
    */
    m_device = AppState::instance().getLogicalDevice();
    auto familyIndex = AppState::instance().getFamilyIndex();

    auto poolInfo = vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, familyIndex );

    try
    {
        m_commandPool = m_device.createCommandPool( poolInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create command pool!" );
    }

    printf( "\nCreated Command Pool\n" );
}

vk::CommandBuffer CommandPool::createBuffer()
{
    /*
    * Record work commands into this
    * Submit this to the queue for the GPU to work on
    */

    auto allocInfo = vk::CommandBufferAllocateInfo(
        m_commandPool, vk::CommandBufferLevel::ePrimary, 1 );

    try
    {
        auto result = m_device.allocateCommandBuffers( allocInfo );
        return result[0];
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate command buffers!" );
    }

    printf( "\nCreated Command Buffer\n" );
}

void CommandPool::destroy()
{
    m_device.destroyCommandPool( m_commandPool );
    m_commandPool = nullptr;
}
