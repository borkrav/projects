#include <BRAppState.h>
#include <BRCommandPool.h>
#include <Util.h>

#include <cassert>

using namespace BR;

CommandPool::CommandPool() : m_commandPool( VK_NULL_HANDLE )
{
}

CommandPool::~CommandPool()
{
    assert( m_commandPool == VK_NULL_HANDLE );
}

void CommandPool::create()
{
    /*
    * Command pools allow concurrent recording of command buffers
    * One pool per thread -> Multi-thread recording
    */

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = AppState::instance().getFamilyIndex();

    VkResult result =
        vkCreateCommandPool( AppState::instance().getLogicalDevice(), &poolInfo,
                             nullptr, &m_commandPool );

    checkSuccess( result );

    printf( "\nCreated Command Pool\n" );
}

VkCommandBuffer CommandPool::createBuffer()
{
    /*
    * Record work commands into this
    * Submit this to the queue for the GPU to work on
    */

    VkCommandBuffer buff;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(
        AppState::instance().getLogicalDevice(), &allocInfo, &buff );

    checkSuccess( result );

    printf( "\nCreated Command Buffer\n" );

    return buff;
}

void CommandPool::destroy()
{
    vkDestroyCommandPool( AppState::instance().getLogicalDevice(),
                          m_commandPool, nullptr );

    m_commandPool = VK_NULL_HANDLE;
}
