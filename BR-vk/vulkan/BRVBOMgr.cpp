#include "BRVBOMgr.h"

#include <cassert>

#include "Util.h"

using namespace BR;

// Finds suitable memory for the vertex buffer
uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties,
                         Device& device )
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties( device.getPhysicaDevice(),
                                         &memProperties );

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

VkBuffer VBOMgr::createBuffer( const std::vector<Pipeline::Vertex>& m_vertices, Device& device )
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof( m_vertices[0] ) * m_vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer( device.getLogicalDevice(), &bufferInfo,
                                      nullptr, &m_vertexBuffer );

    checkSuccess( result );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( device.getLogicalDevice(), m_vertexBuffer,
                                   &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType( memRequirements.memoryTypeBits,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        device );

    result = vkAllocateMemory( device.getLogicalDevice(), &allocInfo, nullptr,
                               &m_vertexBufferMemory );

    vkBindBufferMemory( device.getLogicalDevice(), m_vertexBuffer,
                        m_vertexBufferMemory, 0 );

    checkSuccess( result );

    void* data;
    vkMapMemory( device.getLogicalDevice(), m_vertexBufferMemory, 0,
                 bufferInfo.size, 0, &data );
    memcpy( data, m_vertices.data(), (size_t)bufferInfo.size );
    vkUnmapMemory( device.getLogicalDevice(), m_vertexBufferMemory );

    return m_vertexBuffer;
}

void VBOMgr::destroy( Device& device )
{
    vkDestroyBuffer( device.getLogicalDevice(), m_vertexBuffer, nullptr );
    vkFreeMemory( device.getLogicalDevice(), m_vertexBufferMemory, nullptr );

    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexBufferMemory = VK_NULL_HANDLE;
}
