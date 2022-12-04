#pragma once

#include <BRDevice.h>
#include <BRPipeline.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class RTPipeline : public Pipeline
{
   public:
    RTPipeline(){};
    ~RTPipeline(){};

    void addShaderGroup( vk::RayTracingShaderGroupTypeKHR type,
                         uint32_t index );

    void build( std::string name, vk::DescriptorSetLayout layout );

   private:
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
};
}  // namespace BR
