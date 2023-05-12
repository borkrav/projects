#pragma once

#include <BRRTPipeline.h>
#include <BRRaster.h>

#include "BRASBuilder.h"
#include "BRDescMgr.h"

namespace BR
{

class RayTracer
{
   public:
    RayTracer();

    void init();

    void createAS( std::vector<uint32_t>& indices, vk::Buffer vertexBuffer,
                   vk::Buffer indexBuffer );

    void destroy();

    void createSBT();
    void createRTDescriptorSets( std::vector<vk::Buffer>& uniforms,
                                 vk::DescriptorPool pool,
                                 vk::Buffer vertexBuffer,
                                 vk::Buffer indexBuffer, 
                                 vk::Buffer colorBuffer );

    void setRTRenderTarget( uint32_t imageIndex, int currentFrame );

    void recordRTCommandBuffer( vk::CommandBuffer commandBuffer,
                                uint32_t imageIndex, int currentFrame,
                                Raster& raster );

    void resize();

    void updateTLAS( glm::mat4 model );

   private:
    DescMgr& m_descMgr;
    MemoryMgr& m_bufferAlloc;
    int m_framesInFlight;

    ASBuilder m_asBuilder;

    vk::Buffer m_accBuffer;

    vk::AccelerationStructureKHR m_blas;
    vk::AccelerationStructureKHR m_tlas;

    vk::DescriptorSetLayout m_rtDescriptorSetLayout;
    std::vector<vk::DescriptorSet> m_rtDescriptorSets;

    RTPipeline m_pipeline;

    vk::Device m_device;

    vk::Buffer m_raygenSBT;
    vk::Buffer m_missSBT;
    vk::Buffer m_hitSBT;

    void createAccumulationBuffer();
    void createPipeline();
};
}  // namespace BR
