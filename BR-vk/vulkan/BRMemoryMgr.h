#pragma once

#include <BRCommandPool.h>
#include <BRDevice.h>
#include <BRPipeline.h>
#include <BRUtil.h>

#include <any>
#include <map>
#include <variant>
#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class MemoryMgr
{
    friend class AppState;

   public:

    vk::Buffer createDeviceBuffer(
        std::string name, vk::DeviceSize size, void* srcData, bool hostVisible,
        vk::BufferUsageFlags type = static_cast<vk::BufferUsageFlags>( 0 ) );

    //Update a device buffer ( memcpy data in )
    void updateVisibleBuffer( vk::Buffer buff, vk::DeviceSize size,
                              void* data );

    //Creates image
    vk::Image createImage( std::string name, uint32_t width, uint32_t height,
                           vk::Format format, vk::ImageTiling tiling,
                           vk::ImageUsageFlags usage,
                           vk::MemoryPropertyFlags memFlags );

    vk::ImageView createImageView( std::string name, vk::Image image,
                                   vk::Format format,
                                   vk::ImageAspectFlagBits aspectFlagBits );

    vk::DeviceMemory getMemory( std::variant<vk::Buffer, vk::Image> buffer );
    uint64_t getDeviceAddress( VkBuffer buffer );

    void free( std::variant<vk::Buffer, vk::Image> buffer );

    void destroy();

   private:

    MemoryMgr();
    ~MemoryMgr();

    MemoryMgr( const MemoryMgr& ) = delete;

    // Creates a buffer on the GPU
    std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(
        vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties );

    uint64_t getBufferDeviceAddress( VkBuffer buffer );

    void copyBuffer( vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                     vk::DeviceSize size );

    std::map<std::variant<vk::Buffer, vk::Image>, vk::DeviceMemory> m_alloc;
    std::map<vk::Buffer, uint64_t> m_addresses;
    std::map<vk::Image, std::vector<vk::ImageView> > m_imageViews;

    vk::Device m_device;
    CommandPool m_copyPool;
};
}  // namespace BR
