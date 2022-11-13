#pragma once

#include <BRFramebuffer.h>
#include <BRPipeline.h>
#include <BRRenderPass.h>

#include "BRDescMgr.h"
#include "BRMemoryMgr.h"

namespace BR
{

class Raster
{
   public:
    Raster();

    void init();

    void createDescriptorSets( std::vector<vk::Buffer>& uniforms,
                               vk::DescriptorPool pool );

    void recordDrawCommandBuffer( vk::CommandBuffer commandBuffer,
                                  uint32_t imageIndex, int currentFrame,
                                  vk::Buffer vertexBuffer,
                                  vk::Buffer indexBuffer, int drawCount );

    void destroy();
    void resize();

    vk::RenderPass getRenderPass();

    Framebuffer m_framebuffer;

   private:
    DescMgr& m_descMgr;
    MemoryMgr& m_bufferAlloc;

    vk::Device m_device;

    int m_framesInFlight;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    vk::Image m_depthBuffer;
    vk::ImageView m_depthBufferView;

    RenderPass m_renderPass;
    Pipeline m_pipeline;

    void createDepthBuffer();
};
}  // namespace BR
