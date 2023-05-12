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
    const std::vector<tinyobj::real_t> objNormals = reader.GetAttrib().normals;
    const std::vector<tinyobj::real_t> objCoords = reader.GetAttrib().texcoords;
    const std::vector<tinyobj::shape_t>& objShapes =
        reader.GetShapes();  // All shapes in the file
    const std::vector<tinyobj::material_t>& objMaterials = reader.GetMaterials();

    m_shapes.resize( objShapes.size() );


    // Read obj file
    for ( int s = 0; s < objShapes.size(); ++s )
    {
        auto& indices = objShapes[s].mesh.indices;

        assert( indices.size() % 3 == 0 );

        m_shapes[s].m_name = objShapes[s].name;

        for ( int i = 0; i < indices.size(); i += 3 )
        {
            Vertex tverts[3];

            for ( int j = 0; j < 3; ++j )
            {
                auto vertIndex = indices[i + j].vertex_index;
                auto normIndex = indices[i + j].normal_index;
                auto coordIndex = indices[i + j].texcoord_index;

                assert( vertIndex >= 0 );
                tverts[j].v.x = objVertices[vertIndex * 3];
                tverts[j].v.y = objVertices[vertIndex * 3 + 1];
                tverts[j].v.z = objVertices[vertIndex * 3 + 2];

                if ( normIndex >= 0 )
                {
                    tverts[j].n.x = objNormals[normIndex * 3];
                    tverts[j].n.y = objNormals[normIndex * 3 + 1];
                    tverts[j].n.z = objNormals[normIndex * 3 + 2];
                }

                if ( coordIndex >= 0 )
                {
                    tverts[j].t.x = objCoords[coordIndex * 2];
                    tverts[j].t.y = objCoords[coordIndex * 2 + 1];
                }
                else
                {
                    tverts[j].t.x = -1;
                    tverts[j].t.y = -1;
                }
            }

            int material = objShapes[s].mesh.material_ids[i / 3];

            m_shapes[s].m_triangles.emplace_back(
                Triangle( tverts[0], tverts[1], tverts[2], material ) );
        }
    }

    for( auto& mat : objMaterials )
    {
        m_materials.emplace_back(
            Material( mat.ambient[0], mat.ambient[1], mat.ambient[2],
                      mat.diffuse[0], mat.diffuse[1], mat.diffuse[2],
                      mat.specular[0], mat.specular[1], mat.specular[2] ) );
    }
    

    // prepare vertex buffersM

    std::vector<glm::vec4> rtVertices;
    std::vector<glm::vec4> rtColors;
    int index = 0;

    for ( auto& shape : m_shapes )
    {
        for ( auto& triangle : shape.m_triangles )
        {
            for ( int i = 0; i < 3; ++i )
            {
                Material mat( 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8 );

                if ( !m_materials.empty() )
                    mat = m_materials[triangle.mat];

                PipelineVertex vert{
                    { triangle.verts[i].v.x, triangle.verts[i].v.y,
                      triangle.verts[i].v.z },
                    { triangle.verts[i].n.x, triangle.verts[i].n.y,
                      triangle.verts[i].n.z },
                    { mat.d.r, mat.d.g, mat.d.b } };

                glm::vec4 rawVert = { triangle.verts[i].v.x,
                                      triangle.verts[i].v.y,
                                      triangle.verts[i].v.z, 1 };

                glm::vec4 rtCol = { mat.d.r, mat.d.g, mat.d.b, 1 };

                m_vertices.push_back( vert );
                rtVertices.push_back( rawVert );
                rtColors.push_back( rtCol );
                m_indices.push_back( index );
                index++;
            }
        }
    }

    // std::map<std::string, int> indexMap;      

    // std::vector<glm::vec4> rawVerts;

    // //every combination of vertex/normal pairs must be uniquely defined
    // //data duplication cannot be avoided
    // for ( auto shape : objShapes )
    // {
    //     for ( auto vert : shape.mesh.indices )
    //     {
    //         auto vertIndex = vert.vertex_index;
    //         auto normIndex = vert.normal_index;

    //         PipelineVertex vert{
    //             { objVertices[vertIndex * 3], objVertices[vertIndex * 3 + 1],
    //               objVertices[vertIndex * 3 + 2] },
    //             { objNormals[normIndex * 3], objNormals[normIndex * 3 + 1],
    //               objNormals[normIndex * 3 + 2] } };

    //         glm::vec4 rawVert = { objVertices[vertIndex * 3],
    //                               objVertices[vertIndex * 3 + 1],
    //                               objVertices[vertIndex * 3 + 2], 1 };

    //         m_vertices.push_back( vert );
    //         rawVerts.push_back( rawVert );
    //         m_indices.push_back( index );
    //         index++; 
    //     }
    // }

    auto bufferSize = m_vertices.size() * sizeof( m_vertices[0] );
    m_vertexBuffer = m_bufferAlloc.createDeviceBuffer(
        "Vertex", bufferSize, m_vertices.data(), false,
        vk::BufferUsageFlagBits::eVertexBuffer );

    bufferSize = rtVertices.size() * sizeof( rtVertices[0] );
    m_rtVertexBuffer = m_bufferAlloc.createDeviceBuffer(
        "RTVertex", bufferSize, rtVertices.data(), false,
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::
                eAccelerationStructureBuildInputReadOnlyKHR );

    bufferSize = rtColors.size() * sizeof( rtColors[0] );
    m_rtColorBuffer = m_bufferAlloc.createDeviceBuffer(
        "RTColors", bufferSize, rtColors.data(), false,
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
