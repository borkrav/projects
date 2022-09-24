#include <BRAppState.h>
#include <BRCommandPool.h>
#include <BRUtil.h>

#include <cassert>

using namespace BR;

CommandPool::CommandPool() : m_commandPool( nullptr )
{
}

CommandPool::~CommandPool()
{
    assert( !m_commandPool );
}

void CommandPool::create( std::string name,
                          vk::CommandPoolCreateFlagBits flags )
{
    /*
    * Command pools allow concurrent recording of command buffers
    * One pool per thread -> Multi-thread recording
    */
    m_device = AppState::instance().getLogicalDevice();
    auto familyIndex = AppState::instance().getFamilyIndex();

    auto poolInfo = vk::CommandPoolCreateInfo( flags, familyIndex );

    try
    {
        m_commandPool = m_device.createCommandPool( poolInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create command pool!" );
    }

    DEBUG_NAME( m_commandPool, name );
}

vk::CommandBuffer CommandPool::createBuffer( std::string name )
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

        DEBUG_NAME( result[0], name );

        return result[0];
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate command buffers!" );
    }
}

vk::CommandBuffer CommandPool::beginOneTimeSubmit( std::string name )
{
    assert( m_commandPool );

    vk::CommandBufferAllocateInfo allocInfo(
        m_commandPool, vk::CommandBufferLevel::ePrimary, 1 );

    vk::CommandBuffer result = nullptr;

    try
    {
        auto resultBuffs = m_device.allocateCommandBuffers( allocInfo );
        result = resultBuffs[0];
        DEBUG_NAME( result, name );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate command buffers!" );
    }

    vk::CommandBufferBeginInfo beginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit );

    result.begin( beginInfo );

    return result;
}
void CommandPool::endOneTimeSubmit( vk::CommandBuffer buff )
{
    assert( buff );

    buff.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buff;

    auto queue = AppState::instance().getGraphicsQueue();

    queue.submit( submitInfo );
    queue.waitIdle();

    m_device.freeCommandBuffers( m_commandPool, 1, &buff );
}

void CommandPool::freeBuffer( vk::CommandBuffer buffer )
{
    m_device.freeCommandBuffers( m_commandPool, 1, &buffer );
}

void CommandPool::destroy()
{
    m_device.destroyCommandPool( m_commandPool );
    m_commandPool = nullptr;
}
