#pragma once

#include <BRAppState.h>
#include <BRMemoryMgr.h>

#include <glm/glm.hpp>

namespace BR
{

class Scene
{
   public:
    Scene();

    void loadModel( std::string name );

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 norm;

        // This defines how to read the data
        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof( Vertex );
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;

            return bindingDescription;
        }

        // This describes where the vertex is and where the color is
        // type of color, type of vertex
        // location matches the location inside the shader
        static std::vector<vk::VertexInputAttributeDescription>
        getAttributeDescriptions()
        {
            std::vector<vk::VertexInputAttributeDescription>
                attributeDescriptions( 2,
                                       vk::VertexInputAttributeDescription() );
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[0].offset = offsetof( Vertex, pos );
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[1].offset = offsetof( Vertex, norm );

            return attributeDescriptions;
        }
    };

    MemoryMgr& m_bufferAlloc;

    vk::Buffer m_vertexBuffer;
    vk::Buffer m_rtVertexBuffer;
    vk::Buffer m_indexBuffer;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
};
}  // namespace BR