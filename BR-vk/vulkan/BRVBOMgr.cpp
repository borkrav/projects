#include <BRAppState.h>
#include <BRVBOMgr.h>
#include <Util.h>

#include <cassert>

using namespace BR;

// Finds suitable memory for the vertex buffer
uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties,
                         VkPhysicalDevice device )
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties( device, &memProperties );

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

VkBuffer VBOMgr::createBuffer( const std::vector<Pipeline::Vertex>& m_vertices )
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof( m_vertices[0] ) * m_vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer( AppState::instance().getLogicalDevice(),
                                      &bufferInfo, nullptr, &m_vertexBuffer );

    checkSuccess( result );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( AppState::instance().getLogicalDevice(),
                                   m_vertexBuffer, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        AppState::instance().getPhysicalDevice() );

    result = vkAllocateMemory( AppState::instance().getLogicalDevice(),
                               &allocInfo, nullptr, &m_vertexBufferMemory );

    vkBindBufferMemory( AppState::instance().getLogicalDevice(), m_vertexBuffer,
                        m_vertexBufferMemory, 0 );

    checkSuccess( result );

    void* data;
    vkMapMemory( AppState::instance().getLogicalDevice(), m_vertexBufferMemory,
                 0, bufferInfo.size, 0, &data );
    memcpy( data, m_vertices.data(), (size_t)bufferInfo.size );
    vkUnmapMemory( AppState::instance().getLogicalDevice(),
                   m_vertexBufferMemory );

    return m_vertexBuffer;
}

void VBOMgr::destroy()
{
    vkDestroyBuffer( AppState::instance().getLogicalDevice(), m_vertexBuffer,
                     nullptr );
    vkFreeMemory( AppState::instance().getLogicalDevice(), m_vertexBufferMemory,
                  nullptr );

    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexBufferMemory = VK_NULL_HANDLE;
}
