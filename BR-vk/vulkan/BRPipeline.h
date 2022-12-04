#pragma once

#include <BRDevice.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Pipeline
{
   public:
    Pipeline();
    ~Pipeline();

    void destroy();

    vk::Pipeline get();
    vk::PipelineLayout getLayout();

    void addShaderStage( const std::string& spv,
                         vk::ShaderStageFlagBits stage );

   protected:
    vk::ShaderModule createShaderModule( vk::Device device,
                                         const std::vector<char>& code );

    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_pipeline;
    vk::Device m_device;

    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
    std::vector<vk::ShaderModule> m_shaderModules;
};
}  // namespace BR
