#include <BRAppState.h>
#include <BRVBOMgr.h>
#include <Util.h>

#include <cassert>

using namespace BR;


// TODO: clean this up/rewrite this
// this creates one allocation per buffer, which is fine for a toy example
// should use sub allocations to use the API properly, this is pretty complicated for now


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

VBOMgr::VBOMgr()
    : m_vertexBuffer( VK_NULL_HANDLE ), m_vertexBufferMemory( VK_NULL_HANDLE )
{
}

VBOMgr::~VBOMgr()
{
    assert( m_vertexBuffer == VK_NULL_HANDLE &&
            m_vertexBufferMemory == VK_NULL_HANDLE );
}

std::pair<vk::Buffer, vk::DeviceMemory> createBuf(
    vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties )
{
    auto device = AppState::instance().getLogicalDevice();

    vk::Buffer buffer = nullptr;
    vk::DeviceMemory mem = nullptr;

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    try
    {
        buffer = device.createBuffer( bufferInfo, nullptr );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create vertex buffer!" );
    }

    vk::MemoryRequirements memRequirements =
        device.getBufferMemoryRequirements( buffer );

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits,
                        vk::MemoryPropertyFlagBits::eHostVisible |
                            vk::MemoryPropertyFlagBits::eHostCoherent,
                        AppState::instance().getPhysicalDevice() );

    try
    {
        mem = device.allocateMemory( allocInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate vertex buffer memory!" );
    }

    device.bindBufferMemory( buffer, mem, 0 );

    return std::make_pair( buffer, mem );
}

void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    auto device = AppState::instance().getLogicalDevice();
    auto familyIndex = AppState::instance().getFamilyIndex();
    auto queue = AppState::instance().getGraphicsQueue();

    vk::CommandPool copyPool = nullptr;

    auto poolInfo = vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eTransient, familyIndex );

    try
    {
        copyPool = device.createCommandPool( poolInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create command pool!" );
    }


    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = copyPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer copyBuffer = nullptr;

    try
    {
        auto commandBuffers = device.allocateCommandBuffers( allocInfo );
        copyBuffer = commandBuffers[0];
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create command buffer!" );
    }

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

    device.freeCommandBuffers( copyPool, 1, &copyBuffer );
    device.destroyCommandPool( copyPool );
}

vk::Buffer VBOMgr::createBuffer(
    const std::vector<Pipeline::Vertex>& m_vertices )
{
    auto device = AppState::instance().getLogicalDevice();

    vk::DeviceSize size = sizeof( m_vertices[0] ) * m_vertices.size();

    auto stage = createBuf( size, vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible |
                                 vk::MemoryPropertyFlagBits::eHostCoherent );

    auto stageMem = stage.second;
    auto stageBuffer = stage.first;

    void* data = device.mapMemory( stageMem, 0, size );
    memcpy( data, m_vertices.data(), (size_t)size );
    device.unmapMemory( stageMem );

    auto final = createBuf( size,
                             vk::BufferUsageFlagBits::eTransferDst |
                                 vk::BufferUsageFlagBits::eVertexBuffer,
                             vk::MemoryPropertyFlagBits::eDeviceLocal );

    m_vertexBuffer = final.first;
    m_vertexBufferMemory = final.second;

    copyBuffer(stageBuffer, m_vertexBuffer, size);

    device.destroyBuffer( stageBuffer );
    device.freeMemory( stageMem );

    return m_vertexBuffer;
}


vk::Buffer VBOMgr::createIndexBuffer(
    const std::vector<uint16_t>& m_indices )
{
    auto device = AppState::instance().getLogicalDevice();

    vk::DeviceSize size = sizeof( m_indices[0] ) * m_indices.size();

    auto stage = createBuf( size, vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible |
                                 vk::MemoryPropertyFlagBits::eHostCoherent );

    auto stageMem = stage.second;
    auto stageBuffer = stage.first;

    void* data = device.mapMemory( stageMem, 0, size );
    memcpy( data, m_indices.data(), (size_t)size );
    device.unmapMemory( stageMem );

    auto final = createBuf( size,
                             vk::BufferUsageFlagBits::eTransferDst |
                                 vk::BufferUsageFlagBits::eIndexBuffer,
                             vk::MemoryPropertyFlagBits::eDeviceLocal );

    m_indexBuffer = final.first;
    m_indexBufferMemory = final.second;

    copyBuffer(stageBuffer, m_indexBuffer, size);

    device.destroyBuffer( stageBuffer );
    device.freeMemory( stageMem );

    return m_indexBuffer;
}


void VBOMgr::destroy()
{
    vkDestroyBuffer( AppState::instance().getLogicalDevice(), m_vertexBuffer,
                     nullptr );
    vkFreeMemory( AppState::instance().getLogicalDevice(), m_vertexBufferMemory,
                  nullptr );

    vkDestroyBuffer( AppState::instance().getLogicalDevice(), m_indexBuffer,
                     nullptr );
    vkFreeMemory( AppState::instance().getLogicalDevice(), m_indexBufferMemory,
                  nullptr );

    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexBufferMemory = VK_NULL_HANDLE;
}
