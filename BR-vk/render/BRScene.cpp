#include <BRScene.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace BR;

Scene::Scene() : m_bufferAlloc( AppState::instance().getMemoryMgr() )
{
}

void Scene::loadModel( std::string name )
{
    tinyobj::ObjReader reader;  // Used to read an OBJ file
    reader.ParseFromFile( "models/" + name );

    assert( reader.Valid() );  // Make sure tinyobj was able to parse this file
    const std::vector<tinyobj::real_t> objVertices =
        reader.GetAttrib().GetVertices();
    const std::vector<tinyobj::shape_t>& objShapes =
        reader.GetShapes();  // All shapes in the file

    std::map<std::string, int> indexMap;
    int index = 0;

    std::vector<glm::vec4> rawVerts;

    //every combination of vertex/normal pairs must be uniquely defined
    //data duplication cannot be avoided
    for ( auto shape : objShapes )
    {
        for ( auto vert : shape.mesh.indices )
        {
            auto vertIndex = vert.vertex_index;
            auto normIndex = vert.normal_index;

            Vertex vert{
                { objVertices[vertIndex * 3], objVertices[vertIndex * 3 + 1],
                  objVertices[vertIndex * 3 + 2] },
                { objVertices[normIndex * 3], objVertices[normIndex * 3 + 1],
                  objVertices[normIndex * 3 + 2] } };

            glm::vec4 rawVert = { objVertices[vertIndex * 3],
                                  objVertices[vertIndex * 3 + 1],
                                  objVertices[vertIndex * 3 + 2], 1 };

            //this might be slow
            std::string vertString =
                std::to_string( vert.pos.x ) + std::to_string( vert.pos.y ) +
                std::to_string( vert.pos.z ) + std::to_string( vert.norm.x ) +
                std::to_string( vert.norm.y ) + std::to_string( vert.norm.z );

            auto it = indexMap.find( vertString );

            if ( it == indexMap.end() )
            {
                m_vertices.push_back( vert );
                rawVerts.push_back( rawVert );
                m_indices.push_back( index );
                indexMap[vertString] = index++;
            }
            else
            {
                m_indices.push_back( it->second );
            }
        }
    }

    auto bufferSize = m_vertices.size() * sizeof( m_vertices[0] );
    m_vertexBuffer = m_bufferAlloc.createDeviceBuffer(
        "Vertex", bufferSize, m_vertices.data(), false,
        vk::BufferUsageFlagBits::eVertexBuffer );

    bufferSize = rawVerts.size() * sizeof( rawVerts[0] );
    m_rtVertexBuffer = m_bufferAlloc.createDeviceBuffer(
        "RTVertex", bufferSize, rawVerts.data(), false,
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::
                eAccelerationStructureBuildInputReadOnlyKHR );

    bufferSize = m_indices.size() * sizeof( m_indices[0] );
    m_indexBuffer = m_bufferAlloc.createDeviceBuffer(
        "Index", bufferSize, m_indices.data(), false,
        vk::BufferUsageFlagBits::eIndexBuffer |
            vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::
                eAccelerationStructureBuildInputReadOnlyKHR );
}
