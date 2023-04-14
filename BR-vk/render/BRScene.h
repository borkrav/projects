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

    //This is how we define a vertex in the graphics pipeline - for now, just a position and normal
    struct PipelineVertex
    {
        glm::vec3 pos;
        glm::vec3 norm;

        // This defines how to read the data
        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof( PipelineVertex );
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
            attributeDescriptions[0].offset = offsetof( PipelineVertex, pos );
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[1].offset = offsetof( PipelineVertex, norm );

            return attributeDescriptions;
        }
    };

    //N shapes (Meshes)
    //Each shape has M triangles
    //Each triangle has:
    // 3 vertices
    // 3 normals
    // 3 texture coordinates
    // 1 material ID

    struct Vertex
    {
        glm::vec3 v;
        glm::vec3 n;
        glm::vec2 t;
    };

    struct Triangle
    {
        Triangle( Vertex& v1, Vertex& v2, Vertex& v3, int mat )
            : verts{ std::move( v1 ), std::move( v2 ), std::move( v3 ) },
              mat( mat )
        {
        }
        Vertex verts[3];
        int mat;
    };

    struct Material
    {
        Material( float ar, float ag, float ab, float dr, float dg, float db,
                  float sr, float sg, float sb )
            : a( ar, ag, ab ), d( dr, dg, db ), s( sr, sg, sb )
        {
        }
        glm::vec3 a;
        glm::vec3 d;
        glm::vec3 s;
    };

    struct Shape
    {
        std::string m_name;
        std::vector<Triangle> m_triangles;
    };

    std::vector<Shape> m_shapes;
    std::vector<Material> m_materials;

    MemoryMgr& m_bufferAlloc;

    vk::Buffer m_vertexBuffer;
    vk::Buffer m_rtVertexBuffer;
    vk::Buffer m_indexBuffer;

    std::vector<PipelineVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};
}  // namespace BR