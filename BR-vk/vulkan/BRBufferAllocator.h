#pragma once

#include <BRCommandPool.h>
#include <BRDevice.h>
#include <BRPipeline.h>
#include <BRUtil.h>

#include <map>
#include <variant>
#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class BufferAllocator
{
   public:
    BufferAllocator();
    ~BufferAllocator();

    //Creates a GPU buffer, copies data from CPU to staging buffer, copies from staging to final buffer
    template <typename T, typename A>
    vk::Buffer createAndStageBuffer( std::string name,
                                     std::vector<T, A> const& m_vertices,
                                     vk::BufferUsageFlagBits type = static_cast<vk::BufferUsageFlagBits>(0) )
    {
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

        std::pair<vk::Buffer, vk::DeviceMemory> final;

        // if this has an actual type (vertex buffer, index buffer, etc)
        if ( type != static_cast<vk::BufferUsageFlagBits>(0) )
        {
            final = createBuffer(
                size,
                vk::BufferUsageFlagBits::eTransferDst | type |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress |
                    vk::BufferUsageFlagBits::
                        eAccelerationStructureBuildInputReadOnlyKHR,
                vk::MemoryPropertyFlagBits::eDeviceLocal );
        }

        // if this doesn't have a type (generic storage buffer used for TLAS/BLAS matrices)
        else
        {
            final = createBuffer( size,
                          vk::BufferUsageFlagBits::eTransferDst |
                              vk::BufferUsageFlagBits::eShaderDeviceAddress |
                              vk::BufferUsageFlagBits::
                                  eAccelerationStructureBuildInputReadOnlyKHR,
                          vk::MemoryPropertyFlagBits::eDeviceLocal );
        }

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

    //Creates a buffer for uniforms, host visible/coherent
    vk::Buffer createUniformBuffer( std::string name,
                                    vk::DeviceSize bufferSize );

    //Creates image
    vk::Image createImage( std::string name, uint32_t width, uint32_t height,
                           vk::Format format, vk::ImageTiling tiling,
                           vk::ImageUsageFlags usage,
                           vk::MemoryPropertyFlags memFlags );

    vk::Buffer createAccelStructureBuffer( std::string name,
                                           vk::DeviceSize size );

    vk::Buffer createScratchBuffer( std::string name, vk::DeviceSize size );

    vk::DeviceMemory getMemory( std::variant<vk::Buffer, vk::Image> buffer );
    uint64_t getDeviceAddress( VkBuffer buffer );

    void free( std::variant<vk::Buffer, vk::Image> buffer );

    void destroy();

   private:
    // Creates a buffer on the GPU
    std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(
        vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties );

    uint64_t getBufferDeviceAddress( VkBuffer buffer );

    void copyBuffer( vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                     vk::DeviceSize size );

    std::map<std::variant<vk::Buffer, vk::Image>, vk::DeviceMemory> m_alloc;
    std::map<vk::Buffer, uint64_t> m_addresses;

    vk::Device m_device;
    CommandPool m_copyPool;
};
}  // namespace BR
