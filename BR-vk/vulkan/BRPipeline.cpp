#include <BRAppState.h>
#include <BRPipeline.h>
#include <BRScene.h>
#include <BRUtil.h>

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace BR;

Pipeline::Pipeline() : m_pipeline( nullptr ), m_pipelineLayout( nullptr )
{
}

Pipeline::~Pipeline()
{
    assert( !m_pipeline && !m_pipelineLayout );
}

vk::ShaderModule Pipeline::createShaderModule( vk::Device device,
                                               const std::vector<char>& code )
{
    auto createInfo = vk::ShaderModuleCreateInfo();
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );

    try
    {
        vk::ShaderModule shaderModule = device.createShaderModule( createInfo );
        return shaderModule;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create shader module!" );
    }
}

void Pipeline::addShaderStage( const std::string& spv,
                               vk::ShaderStageFlagBits stage )
{
    std::cout << "\nCurrent load path is: " << std::filesystem::current_path()
              << std::endl;

    m_device = AppState::instance().getLogicalDevice();

    auto code = readFile( spv );

    vk::ShaderModule shaderModule = createShaderModule( m_device, code );

    vk::PipelineShaderStageCreateInfo info(
        vk::PipelineShaderStageCreateFlags(), stage, shaderModule, "main" );

    m_shaderStages.push_back( info );
    m_shaderModules.push_back( shaderModule );
}

void Pipeline::destroy()
{
    m_device.destroyPipeline( m_pipeline );
    m_device.destroyPipelineLayout( m_pipelineLayout );

    m_pipeline = nullptr;
    m_pipelineLayout = nullptr;
}

vk::Pipeline Pipeline::get()
{
    assert( m_pipeline );
    return m_pipeline;
}

vk::PipelineLayout Pipeline::getLayout()
{
    assert( m_pipelineLayout );
    return m_pipelineLayout;
}
