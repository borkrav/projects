#include <BRAppState.h>
#include <BRBufferAllocator.h>
#include <BRUtil.h>

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

    vk::MemoryAllocateFlagsInfo allocFlags;
    allocFlags.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits, properties,
                        AppState::instance().getPhysicalDevice() );
    allocInfo.pNext = &allocFlags;

    try
    {
        mem = m_device.allocateMemory( allocInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate vertex buffer memory!" );
    }

    m_device.bindBufferMemory( buffer, mem, 0 );

    // if we want the address, save it
    if ( (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) == vk::BufferUsageFlagBits::eShaderDeviceAddress )
    {
        auto address = getBufferDeviceAddress( buffer );
        m_addresses[buffer] = address;
    }

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

vk::Buffer BufferAllocator::createUniformBuffer( std::string name,
                                                 vk::DeviceSize bufferSize )
{
    auto result =
        createBuffer( bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent );

    DEBUG_NAME( result.first, name );

    m_alloc[result.first] = result.second;

    return result.first;
}

vk::Image BufferAllocator::createImage( std::string name, uint32_t width,
                                        uint32_t height, vk::Format format,
                                        vk::ImageTiling tiling,
                                        vk::ImageUsageFlags usage,
                                        vk::MemoryPropertyFlags memFlags )
{
    if ( !m_device )
    {
        m_device = AppState::instance().getLogicalDevice();
        m_copyPool.create( "Buffer Copy Pool",
                           vk::CommandPoolCreateFlagBits::eTransient );
    }

    vk::Image image = nullptr;
    vk::DeviceMemory mem = nullptr;

    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    try
    {
        image = m_device.createImage( imageInfo, nullptr );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create image!" );
    }

    vk::MemoryRequirements memRequirements =
        m_device.getImageMemoryRequirements( image );

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits, memFlags,
                        AppState::instance().getPhysicalDevice() );

    try
    {
        mem = m_device.allocateMemory( allocInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to allocate image buffer memory!" );
    }

    m_device.bindImageMemory( image, mem, 0 );

    m_alloc[image] = mem;

    DEBUG_NAME( image, name );

    return image;
}

vk::Buffer BufferAllocator::createAccelStructureBuffer( std::string name,
                                                        vk::DeviceSize size )
{
    auto result = createBuffer(
        size,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal );

    auto buff = result.first;
    auto mem = result.second;

    m_alloc[buff] = mem;

    DEBUG_NAME( buff, name );

    return buff;
}

vk::Buffer BufferAllocator::createScratchBuffer( std::string name,
                                                 vk::DeviceSize size )
{
    auto result =
        createBuffer( size,
                      vk::BufferUsageFlagBits::eStorageBuffer |
                          vk::BufferUsageFlagBits::eShaderDeviceAddress,
                      vk::MemoryPropertyFlagBits::eDeviceLocal );

    auto buff = result.first;
    auto mem = result.second;

    m_alloc[buff] = mem;

    DEBUG_NAME( buff, name );

    return buff;
}

vk::DeviceMemory BufferAllocator::getMemory(
    std::variant<vk::Buffer, vk::Image> buffer )
{
    auto it = m_alloc.find( buffer );

    assert( it != m_alloc.end() );

    return it->second;
}

uint64_t BufferAllocator::getDeviceAddress( VkBuffer buffer )
{
    auto it = m_addresses.find( buffer );
    assert( it != m_addresses.end() );
    return it->second;
}

uint64_t BufferAllocator::getBufferDeviceAddress( VkBuffer buffer )
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return AppState::instance().vkGetBufferDeviceAddressKHR( m_device,
                                                             &bufferDeviceAI );
}

void BufferAllocator::free( std::variant<vk::Buffer, vk::Image> buffer )
{
    auto it = m_alloc.find( buffer );
    assert( it != m_alloc.end() );

    auto obj = it->first;
    auto mem = it->second;

    if ( std::holds_alternative<vk::Buffer>( obj ) )
        m_device.destroyBuffer( std::get<vk::Buffer>( obj ) );

    else if ( std::holds_alternative<vk::Image>( obj ) )
        m_device.destroyImage( std::get<vk::Image>( obj ) );

    m_device.freeMemory( mem );

    m_alloc.erase( it );

    //delete the address
    if ( std::holds_alternative<vk::Buffer>( obj ) )
    {
        auto it2 = m_addresses.find( std::get<vk::Buffer>( buffer ) );
        if ( it2 != m_addresses.end() )
            m_addresses.erase( it2 );
    }
}

void BufferAllocator::destroy()
{
    for ( auto alloc : m_alloc )
    {
        auto obj = alloc.first;
        auto mem = alloc.second;

        if ( std::holds_alternative<vk::Buffer>( obj ) )
            m_device.destroyBuffer( std::get<vk::Buffer>( obj ) );

        else if ( std::holds_alternative<vk::Image>( obj ) )
            m_device.destroyImage( std::get<vk::Image>( obj ) );

        m_device.freeMemory( mem );
    }

    m_copyPool.destroy();

    m_alloc.clear();
}
