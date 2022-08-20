#include <BRAppState.h>
#include <BRBufferAllocator.h>
#include <Util.h>

#include <cassert>

using namespace BR;

// This creates one vk::DeviceMemory per vk::Buffer
// In production, this won't work, because of limit on number of vk::DeviceMemory
// I may implement suballcation/memory chunks later, or use the AMD VMA library
// For this educational renderer, the current strategy should work fine


// Finds suitable memory for the vertex buffer
uint32_t findMemoryType( uint32_t typeFilter,
                         vk::MemoryPropertyFlags properties,
                         vk::PhysicalDevice device )
{
    vk::PhysicalDeviceMemoryProperties memProperties =
        device.getMemoryProperties();

    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
    {
        if ( ( typeFilter & ( 1 << i ) ) &&
             ( memProperties.memoryTypes[i].propertyFlags & properties ) ==
                 properties )
        {
            return i;
        }
    }

    assert( false );
    return -1;
}

BufferAllocator::BufferAllocator() : m_device( nullptr )
{
}

BufferAllocator::~BufferAllocator()
{
    assert( m_alloc.empty() );
}

std::pair<vk::Buffer, vk::DeviceMemory> BufferAllocator::createBuffer(
    vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties )
{
    if ( !m_device )
    {
        m_device = AppState::instance().getLogicalDevice();
        m_copyPool.create( "Buffer Copy Pool",
                           vk::CommandPoolCreateFlagBits::eTransient );
    }

    vk::Buffer buffer = nullptr;
    vk::DeviceMemory mem = nullptr;

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;  
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    try
    {
        buffer = m_device.createBuffer( bufferInfo, nullptr );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create vertex buffer!" );
    }

    vk::MemoryRequirements memRequirements =
        m_device.getBufferMemoryRequirements( buffer );

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits,
                        vk::MemoryPropertyFlagBits::eHostVisible |
                            vk::MemoryPropertyFlagBits::eHostCoherent,
                        AppState::instance().getPhysicalDevice() );

    try
    {
        mem = m_device.allocateMemory( allocInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate vertex buffer memory!" );
    }

    m_device.bindBufferMemory( buffer, mem, 0 );

    return std::make_pair( buffer, mem );
}

void BufferAllocator::copyBuffer( vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                         vk::DeviceSize size )
{
    auto familyIndex = AppState::instance().getFamilyIndex();
    auto queue = AppState::instance().getGraphicsQueue();

    auto copyBuffer = m_copyPool.createBuffer( "Buffer copy buffer" );

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    copyBuffer.begin( beginInfo );
    vk::BufferCopy copyRegion( 0, 0, size );
    copyBuffer.copyBuffer( srcBuffer, dstBuffer, copyRegion );
    copyBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyBuffer;

    queue.submit( submitInfo );
    queue.waitIdle();

    m_copyPool.freeBuffer( copyBuffer );
}


vk::Buffer BufferAllocator::createUniformBuffer(vk::DeviceSize bufferSize)
{
    auto result =
        createBuffer( bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent );

    m_alloc[result.first] = result.second;

    return result.first;
}

vk::DeviceMemory BufferAllocator::getMemory(vk::Buffer buffer)
{
    assert( m_alloc.find( buffer ) != m_alloc.end() );

    return m_alloc[buffer];
}

void BufferAllocator::destroy()
{
    for ( auto alloc : m_alloc )
    {
        m_device.destroyBuffer( alloc.first );
        m_device.freeMemory( alloc.second );
    }

    m_copyPool.destroy();

    m_alloc.clear();
}
