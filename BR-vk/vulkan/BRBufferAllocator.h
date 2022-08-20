#pragma once

#include <BRCommandPool.h>
#include <BRDevice.h>
#include <BRPipeline.h>

#include <map>
#include <vector>
#include <vulkan/vulkan_handles.hpp>
#include <Util.h>

namespace BR
{
class BufferAllocator
{
   public:
    BufferAllocator();
    ~BufferAllocator();

    template <typename T, typename A>
    vk::Buffer createAndStageBuffer( std::string name,
                                     std::vector<T, A> const& m_vertices,
                                     vk::BufferUsageFlagBits type )
    {
        if ( !m_device )
        {
            m_device = AppState::instance().getLogicalDevice();
            m_copyPool.create( "Buffer Copy Pool",
                               vk::CommandPoolCreateFlagBits::eTransient );
        }

        vk::DeviceSize size = sizeof( m_vertices[0] ) * m_vertices.size();

        auto stage =
            createBuffer( size, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible |
                              vk::MemoryPropertyFlagBits::eHostCoherent );

        auto stageMem = stage.second;
        auto stageBuffer = stage.first;

        DEBUG_NAME( stageMem, "stageMem " + name );
        DEBUG_NAME( stageBuffer, "stageBuffer " + name );

        void* data = m_device.mapMemory( stageMem, 0, size );
        memcpy( data, m_vertices.data(), (size_t)size );
        m_device.unmapMemory( stageMem );

        auto final =
            createBuffer( size, vk::BufferUsageFlagBits::eTransferDst | type,
                          vk::MemoryPropertyFlagBits::eDeviceLocal );
        auto resultBuffer = final.first;
        auto resultMem = final.second;

        copyBuffer( stageBuffer, resultBuffer, size );

        m_device.destroyBuffer( stageBuffer );
        m_device.freeMemory( stageMem );

        m_alloc[resultBuffer] = resultMem;

        DEBUG_NAME( resultMem, "mem " + name );
        DEBUG_NAME( resultBuffer, "buffer " + name );

        return resultBuffer;
    }

    void destroy();

   private:
    std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(
        vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties );

    void copyBuffer( vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                     vk::DeviceSize size );

    std::map<vk::Buffer, vk::DeviceMemory> m_alloc;

    vk::Device m_device;
    CommandPool m_copyPool;
};
}  // namespace BR
