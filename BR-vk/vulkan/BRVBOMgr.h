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

    VkBuffer createBuffer( const std::vector<Pipeline::Vertex>& m_vertices );
    void destroy();

   private:
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
};
}  // namespace BR
