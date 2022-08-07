#pragma once

#include <BRDevice.h>
#include <BRPipeline.h>

#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class VBOMgr
{
   public:
    VBOMgr();
    ~VBOMgr();

    vk::Buffer createBuffer( const std::vector<Pipeline::Vertex>& m_vertices );
    vk::Buffer createIndexBuffer( const std::vector<uint16_t>& m_indices );
    void destroy();

   private:
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
};
}  // namespace BR
