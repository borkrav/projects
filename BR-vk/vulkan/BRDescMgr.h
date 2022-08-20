#pragma once

#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class DescMgr
{
   public:
    DescMgr();
    ~DescMgr();

    struct Binding
    {
        int bind;
        vk::DescriptorType type;
        int count;
        vk::ShaderStageFlagBits stage;
    };

    struct PoolSize
    {
        vk::DescriptorType type;
        uint32_t count;
    };

    void destroy();

    vk::DescriptorSetLayout createLayout( std::string name,
                                          const std::vector<Binding>& params );

    vk::DescriptorPool createPool( std::string name, int numSets,
                                   const std::vector<PoolSize>& sizes );

    vk::DescriptorSet createSet( std::string name,
                                 vk::DescriptorSetLayout layout,
                                 vk::DescriptorPool pool );

   private:
    vk::Device m_device;
    std::vector<vk::DescriptorSetLayout> m_layouts;
    std::vector<vk::DescriptorPool> m_pools;
};
}  // namespace BR
