#include <BRAppState.h>
#include <BRRTPipeline.h>
#include <BRScene.h>
#include <BRUtil.h>

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace BR;

void RTPipeline::addShaderGroup( vk::RayTracingShaderGroupTypeKHR type,
                                 uint32_t index )
{
    vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
    shaderGroup.type = type;
    shaderGroup.generalShader = index;
    shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    m_shaderGroups.push_back( shaderGroup );
}

void RTPipeline::build( std::string name, vk::DescriptorSetLayout layout )
{
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;

    try
    {
        m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create pipeline layout!" );
    }

    DEBUG_NAME( m_pipelineLayout, "RT Pipeline Layout" );

    vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineInfo;
    rayTracingPipelineInfo.stageCount = m_shaderStages.size();
    rayTracingPipelineInfo.pStages = m_shaderStages.data();
    rayTracingPipelineInfo.groupCount =
        static_cast<uint32_t>( m_shaderGroups.size() );
    rayTracingPipelineInfo.pGroups = m_shaderGroups.data();
    rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineInfo.layout = m_pipelineLayout;

    auto result = AppState::instance().vkCreateRayTracingPipelinesKHR(
        m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
        reinterpret_cast<VkRayTracingPipelineCreateInfoKHR*>(
            &rayTracingPipelineInfo ),
        nullptr, reinterpret_cast<VkPipeline*>( &m_pipeline ) );

    checkSuccess( result );

    DEBUG_NAME( m_pipeline, name );

    for ( auto& module : m_shaderModules )
        m_device.destroyShaderModule( module );
}
