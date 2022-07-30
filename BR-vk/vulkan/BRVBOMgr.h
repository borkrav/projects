#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "BRDevice.h"
#include "BRPipeline.h"

namespace BR
{
class VBOMgr
{
   public:
    VBOMgr();
    ~VBOMgr();

    VkBuffer createBuffer( const std::vector<Pipeline::Vertex>& m_vertices, Device& device );
    void destroy( Device& device );

   private:
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
};
}  // namespace BR
