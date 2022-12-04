#include <BRAppState.h>
#include <BRRasterPipeline.h>
#include <BRScene.h>
#include <BRUtil.h>

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace BR;

void RasterPipeline::addVertexInputInfo(
    vk::VertexInputBindingDescription& binding,
    std::vector<vk::VertexInputAttributeDescription>& attributes )
{
    m_vertexInputInfo.vertexBindingDescriptionCount = 1;
    m_vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>( attributes.size() );
    m_vertexInputInfo.pVertexBindingDescriptions = &binding;
    m_vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
}

void RasterPipeline::addInputAssembly( vk::PrimitiveTopology topology,
                                       bool restart )
{
    m_inputAssembly.topology = topology;
    m_inputAssembly.primitiveRestartEnable = restart;
}

void RasterPipeline::addViewport( vk::Extent2D extent )
{
    vk::Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = extent;

    m_viewportState.viewportCount = 1;
    m_viewportState.pViewports = &viewport;
    m_viewportState.scissorCount = 1;
    m_viewportState.pScissors = &scissor;
}

void RasterPipeline::addRasterizer( vk::CullModeFlagBits cull,
                                    vk::FrontFace frontFace )
{
    m_rasterizer.depthClampEnable = VK_FALSE;
    m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    m_rasterizer.polygonMode = vk::PolygonMode::eFill;
    m_rasterizer.lineWidth = 1.0f;
    m_rasterizer.cullMode = cull;
    m_rasterizer.frontFace = frontFace;
    m_rasterizer.depthBiasEnable = VK_FALSE;
}

void RasterPipeline::addDepthSencil( vk::CompareOp op )
{
    m_depthStencil.depthTestEnable = VK_TRUE;
    m_depthStencil.depthWriteEnable = VK_TRUE;
    m_depthStencil.depthCompareOp = op;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.minDepthBounds = 0.0f;  // Optional
    m_depthStencil.maxDepthBounds = 1.0f;  // Optional
    m_depthStencil.stencilTestEnable = VK_FALSE;
}

void RasterPipeline::addMultisampling( vk::SampleCountFlagBits samples )
{
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = samples;
}

void RasterPipeline::addColorBlend()
{
    m_colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    m_colorBlendAttachment.blendEnable = VK_FALSE;

    m_colorBlend.logicOpEnable = VK_FALSE;
    m_colorBlend.logicOp = vk::LogicOp::eCopy;
    m_colorBlend.attachmentCount = 1;
    m_colorBlend.pAttachments = &m_colorBlendAttachment;
    m_colorBlend.blendConstants[0] = 0.0f;
    m_colorBlend.blendConstants[1] = 0.0f;
    m_colorBlend.blendConstants[2] = 0.0f;
    m_colorBlend.blendConstants[3] = 0.0f;
}

void RasterPipeline::addDynamicStates( std::vector<vk::DynamicState> states )
{
    m_dynamicStates = states;
}

void RasterPipeline::build( std::string name, RenderPass& renderpass,
                            vk::DescriptorSetLayout layout )
{
    assert( !m_pipeline && !m_pipelineLayout );

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    try
    {
        m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create pipeline layout!" );
    }

    DEBUG_NAME( m_pipelineLayout, name + "layout" );

    vk::PipelineDynamicStateCreateInfo dynamicInfo;
    dynamicInfo.dynamicStateCount = m_dynamicStates.size();
    dynamicInfo.pDynamicStates = m_dynamicStates.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = m_shaderStages.size();
    pipelineInfo.pStages = m_shaderStages.data();
    pipelineInfo.pVertexInputState = &m_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &m_viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
    pipelineInfo.pDepthStencilState = &m_depthStencil;
    pipelineInfo.pMultisampleState = &m_multisampling;
    pipelineInfo.pColorBlendState = &m_colorBlend;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderpass.get();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.pDynamicState = &dynamicInfo;

    try
    {
        auto pipe = m_device.createGraphicsPipeline( nullptr, pipelineInfo );
        m_pipeline = pipe;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create graphics pipeline!" );
    }

    for ( auto& module : m_shaderModules )
        m_device.destroyShaderModule( module );

    DEBUG_NAME( m_pipeline, name )
}

/*  Vulkan Graphics Pipeline ( Vulkan Programming Guide Book )
Some from https://www.scratchapixel.com/index.php

Fixed function stages may be implemented as shaders, depending on driver


Draw
    - The start of pipeline
    - Small processor on the GPU interperts commands in Command Buffer
    - Interacts with hardware to do work

Input Assembly
    - Read index and vertex buffers
    - How indices/vertices are read is specified by the topology

    - List topology - points ( single vertices ), lines ( pairs of vertices ), triangles ( triplets of vertices )
    - Strip/Fans - line strip, triangle strip, triangle fan
    - Adjacency primitives - convey additional information to geometry shader - info about adjacent primitives
    - Topology patch list - a topology used when tesselation is enabled
    - Primitive restart allows multiple strips/fans to be drawn in a single draw call

Vertex Shader
    - Vertex shader executes, transforms/processes vertex data

Tessellation Primitive Generation
    - Fixed function, breaks patch primitives into smaller primitives

Tessellation evaluation shader
    - Runs on each generated vertex. Similar to vertex shader, except the data is generated instead of read from memory

Geometry Shader
    - Operates on full primitives ( points, lines or triagles )
    - Can change the primitive type mid-pipeline, ie convert triangles to lines

Primitive Assembly
    - Groups vertices produced by Vertex/Tessellation/Geometry into primitives suitable for rasterization
    - Culls and clips primitives and transforms them into viewport

Clip and Cull
    - Fixed function, determines which primitives might contribute to the image, discards everything else
    - Forwards visible primitives to rasterizer
    
    - Test each vertex against each viewport plane (6 sides)
    - Inside, keep
    - Outside, cull
    - Both, clip ( break down into smaller primitives ) 

    - After this we have normalized clip coordinates
    - Transform clip coordinates to normalized device coordinates ( We're still in 3D), this is between -1 and 1
    - Transfrom NDC to framebuffer coordinates ( relative to the origin of the framebuffer )

Rasterizer
    - Takes assembled primitives (sequence of vertices) and turns them into individial fragments

    - A fragment is the set of data neccessary to generate a single pixel
        - raster position
        - depth
        - color/texture/etc interpolation
        - stencil
        - alpha

    - Projects triangles onto the screen, using perspective projection
    - Figure out which pixels the triangle covers, color the pixels
    - We have to iterate over every triangle in the scene
    
    "Rasterization is the process of determining which pixels are inside a triangle, and nothing more."





    - depthClampEnable
    - fragments that would have been clipped by near/far planes instead get projected onto those planes
    - polygon capping

    - Culling
    - Throw away front or back faces
    - Depends on the winding order of vertices

    - Depth Biasing
    - Allows fragments to be offset in depth before running the test - fixes z-fighting

   
    


Pregragment operations
    - Performs depth and stencil tests once fragment positions are known

    - Most implementations prefer to run these before the fragment shader, because this avoids shader code exectution
        on failed fragments

Fragment Assembly
    - Takes output of rasterizer, sends to fragment shading stage

Fragment Shader
    - Computes data that will be sent to final stages

Postfragment operations
    - Sometimes, the fragment shader may modify data used in pre-fragment operations ( depth and stencil )
    - Those operations execute here, after the fragment shader, instead of before

Color Blending
    - Color operations take final result + post fragment operations and use them to update framebuffer
    - Blending and logic operations

    - Blending allows mixing output pixels with pixels already in the framebuffer, instead of overwriting






























*/