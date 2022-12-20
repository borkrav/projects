#include "BRRayTracer.h"

#include <BRRender.h>

#include <ranges>

#include "BRAppState.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

using namespace BR;

RayTracer::RayTracer()
    : m_bufferAlloc( AppState::instance().getMemoryMgr() ),
      m_descMgr( AppState::instance().getDescMgr() ),
      m_framesInFlight( AppState::instance().m_framesInFlight )
{
    m_device = AppState::instance().getLogicalDevice();
}

void RayTracer::init()
{
    m_asBuilder.create();

    createAccumulationBuffer();

    m_rtDescriptorSetLayout = m_descMgr.createLayout(
        "RT Pipeline Descriptor Set Layout",
        std::vector<BR::DescMgr::Binding>{
            { 0, vk::DescriptorType::eAccelerationStructureKHR, 1,
              vk::ShaderStageFlagBits::eRaygenKHR },
            { 1, vk::DescriptorType::eStorageImage, 1,
              vk::ShaderStageFlagBits::eRaygenKHR },
            { 2, vk::DescriptorType::eUniformBuffer, 1,
              vk::ShaderStageFlagBits::eRaygenKHR },
            { 3, vk::DescriptorType::eStorageBuffer, 1,
              vk::ShaderStageFlagBits::eClosestHitKHR },
            { 4, vk::DescriptorType::eStorageBuffer, 1,
              vk::ShaderStageFlagBits::eClosestHitKHR },
            { 5, vk::DescriptorType::eStorageBuffer, 1,
              vk::ShaderStageFlagBits::eRaygenKHR } } );

    createPipeline();
}

void RayTracer::createPipeline()
{
    m_pipeline.addShaderStage( "build/shaders/raygen.rgen.spv",
                               vk::ShaderStageFlagBits::eRaygenKHR );
    m_pipeline.addShaderStage( "build/shaders/miss.rmiss.spv",
                               vk::ShaderStageFlagBits::eMissKHR );
    m_pipeline.addShaderStage( "build/shaders/closesthit.rchit.spv",
                               vk::ShaderStageFlagBits::eClosestHitKHR );

    m_pipeline.addShaderGroup( vk::RayTracingShaderGroupTypeKHR::eGeneral, 0 );
    m_pipeline.addShaderGroup( vk::RayTracingShaderGroupTypeKHR::eGeneral, 1 );
    m_pipeline.addShaderGroup(
        vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, 2 );

    m_pipeline.build( "RT Pipeline", m_rtDescriptorSetLayout );
}

void RayTracer::createAS( std::vector<uint32_t>& indices,
                          vk::Buffer vertexBuffer, vk::Buffer indexBuffer )
{
    auto it = std::max_element( indices.begin(), indices.end() );
    int maxVertex = ( *it ) + 1;

    m_blas = m_asBuilder.buildBlas( "BLAS", vertexBuffer, indexBuffer,
                                    maxVertex, indices.size() );

    m_tlas = m_asBuilder.buildTlas( "TLAS", m_blas );
}

void RayTracer::createAccumulationBuffer()
{
    auto extent = AppState::instance().getSwapchainExtent();
    vk::DeviceSize size = extent.width * extent.height * sizeof( glm::vec4 );

    m_accBuffer = m_bufferAlloc.createDeviceBuffer(
        "Accumulation Buffer", size, nullptr, false,
        vk::BufferUsageFlagBits::eStorageBuffer );
}

void RayTracer::createSBT()
{
    // size, in bytes, of the shader handle
    const uint32_t handleSize =
        AppState::instance().rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAlignment =
        AppState::instance()
            .rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned =
        alignedSize( handleSize, handleSizeAlignment );
    const uint32_t groupCount =
        3;  //TODO: This is how many shaders I'm using for the RT Pipeline
    const uint32_t sbtSize = groupCount * handleSizeAligned;

    //3 32 byte handles
    std::vector<uint8_t> shaderHandleStorage( sbtSize );

    //These are the addresses for where the shaders are
    AppState::instance().vkGetRayTracingShaderGroupHandlesKHR(
        m_device, m_pipeline.get(), 0, groupCount, sbtSize,
        shaderHandleStorage.data() );

    const vk::BufferUsageFlags bufferUsageFlags =
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    //create 3 buffers, each holding the address of the shader
    m_raygenSBT = m_bufferAlloc.createDeviceBuffer( "RayGen SBT", handleSize,
                                                    shaderHandleStorage.data(),
                                                    true, bufferUsageFlags );
    m_missSBT = m_bufferAlloc.createDeviceBuffer(
        "Miss SBT", handleSize, shaderHandleStorage.data() + handleSizeAligned,
        true, bufferUsageFlags );
    m_hitSBT = m_bufferAlloc.createDeviceBuffer(
        "Hit SBT", handleSize,
        shaderHandleStorage.data() + handleSizeAligned * 2, true,
        bufferUsageFlags );
}

void RayTracer::createRTDescriptorSets( std::vector<vk::Buffer>& uniforms,
                                        vk::DescriptorPool pool,
                                        vk::Buffer vertexBuffer,
                                        vk::Buffer indexBuffer )
{
    m_rtDescriptorSets.push_back(
        m_descMgr.createSet( "RT Desc Set 1", m_rtDescriptorSetLayout, pool ) );
    m_rtDescriptorSets.push_back(
        m_descMgr.createSet( "RT Desc Set 2", m_rtDescriptorSetLayout, pool ) );

    for ( int i : std::views::iota( 0, m_framesInFlight ) )
    {
        //Acceleration Structure
        vk::WriteDescriptorSetAccelerationStructureKHR asInfo;
        asInfo.accelerationStructureCount = 1;
        asInfo.pAccelerationStructures = &m_tlas;

        vk::WriteDescriptorSet asWrite;
        // The specialized acceleration structure descriptor has to be chained
        asWrite.pNext = &asInfo;
        asWrite.dstSet = m_rtDescriptorSets[i];
        asWrite.dstBinding = 0;
        asWrite.descriptorCount = 1;
        asWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

        //The uniform Buffer, same as for raster
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = uniforms[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( BRRender::UniformBufferObject );

        vk::WriteDescriptorSet uniformBufferWrite;
        uniformBufferWrite.dstSet = m_rtDescriptorSets[i];
        uniformBufferWrite.dstBinding = 2;
        uniformBufferWrite.dstArrayElement = 0;
        uniformBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        uniformBufferWrite.descriptorCount = 1;

        uniformBufferWrite.pBufferInfo = &bufferInfo;
        uniformBufferWrite.pImageInfo = nullptr;        // Optional
        uniformBufferWrite.pTexelBufferView = nullptr;  // Optional

        vk::DescriptorBufferInfo vertBufferInfo;
        vertBufferInfo.buffer = vertexBuffer;
        vertBufferInfo.offset = 0;
        vertBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet vertexBufferWrite;
        vertexBufferWrite.dstSet = m_rtDescriptorSets[i];
        vertexBufferWrite.dstBinding = 3;
        vertexBufferWrite.dstArrayElement = 0;
        vertexBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        vertexBufferWrite.descriptorCount = 1;
        vertexBufferWrite.pBufferInfo = &vertBufferInfo;
        vertexBufferWrite.pImageInfo = nullptr;        // Optional
        vertexBufferWrite.pTexelBufferView = nullptr;  // Optional

        vk::DescriptorBufferInfo indexBufferInfo;
        indexBufferInfo.buffer = indexBuffer;
        indexBufferInfo.offset = 0;
        indexBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet indexBufferWrite;
        indexBufferWrite.dstSet = m_rtDescriptorSets[i];
        indexBufferWrite.dstBinding = 4;
        indexBufferWrite.dstArrayElement = 0;
        indexBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        indexBufferWrite.descriptorCount = 1;
        indexBufferWrite.pBufferInfo = &indexBufferInfo;
        indexBufferWrite.pImageInfo = nullptr;        // Optional
        indexBufferWrite.pTexelBufferView = nullptr;  // Optional

        vk::DescriptorBufferInfo accelBufferInfo;
        accelBufferInfo.buffer = m_accBuffer;
        accelBufferInfo.offset = 0;
        accelBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet accelBufferWrite;
        accelBufferWrite.dstSet = m_rtDescriptorSets[i];
        accelBufferWrite.dstBinding = 5;
        accelBufferWrite.dstArrayElement = 0;
        accelBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        accelBufferWrite.descriptorCount = 1;
        accelBufferWrite.pBufferInfo = &accelBufferInfo;
        accelBufferWrite.pImageInfo = nullptr;        // Optional
        accelBufferWrite.pTexelBufferView = nullptr;  // Optional

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
            asWrite, uniformBufferWrite, vertexBufferWrite, indexBufferWrite,
            accelBufferWrite };

        vkUpdateDescriptorSets(
            m_device, writeDescriptorSets.size(), (VkWriteDescriptorSet*)writeDescriptorSets.data(), 0,
            nullptr );
    }
}

void RayTracer::setRTRenderTarget( uint32_t imageIndex, int currentFrame )
{
    auto views = AppState::instance().getImageViews();

    //the render target
    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageView = views[imageIndex];
    imageInfo.imageLayout = vk::ImageLayout::eGeneral;

    vk::WriteDescriptorSet write;
    write.dstSet = m_rtDescriptorSets[currentFrame];
    write.dstBinding = 1;
    write.dstArrayElement = 0;
    write.descriptorType = vk::DescriptorType::eStorageImage;
    write.descriptorCount = 1;

    write.pBufferInfo = nullptr;
    write.pImageInfo = &imageInfo;     // Optional
    write.pTexelBufferView = nullptr;  // Optional

    vkUpdateDescriptorSets( m_device, 1, (VkWriteDescriptorSet*)&write, 0,
                            nullptr );
}

void RayTracer::recordRTCommandBuffer( vk::CommandBuffer commandBuffer,
                                       uint32_t imageIndex, int currentFrame,
                                       Raster& raster )
{
    auto extent = AppState::instance().getSwapchainExtent();

    vk::Image srcImage = AppState::instance().getSwapchainImage( imageIndex );

    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    // memory read -> transfer read
    // undefined -> transfer source
    imageBarrier( commandBuffer, srcImage, range,
                  vk::AccessFlagBits::eMemoryRead,
                  vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eGeneral );

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eRayTracingKHR,
                                m_pipeline.get() );

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        m_pipeline.getLayout(), 0, 1,
        (VkDescriptorSet*)&m_rtDescriptorSets[currentFrame], 0, nullptr );

    const uint32_t handleSize =
        AppState::instance().rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAlignment =
        AppState::instance()
            .rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned =
        alignedSize( handleSize, handleSizeAlignment );

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress =
        m_bufferAlloc.getDeviceAddress( m_raygenSBT );
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress =
        m_bufferAlloc.getDeviceAddress( m_missSBT );
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress =
        m_bufferAlloc.getDeviceAddress( m_hitSBT );
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    //Ray Trace
    AppState::instance().vkCmdTraceRaysKHR(
        commandBuffer, &raygenShaderSbtEntry, &missShaderSbtEntry,
        &hitShaderSbtEntry, &callableShaderSbtEntry, extent.width,
        ( extent.height / 2 ) * 2, 1 );
    //Height needs to be multiple of 2, not sure why, otherwise accumulation breaks
    //TODO: Figure out why this is happening

    imageBarrier( commandBuffer, srcImage, range,
                  vk::AccessFlagBits::eShaderWrite,
                  vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::eGeneral,
                  vk::ImageLayout::eGeneral );
}

void RayTracer::resize()
{
    m_bufferAlloc.free( m_accBuffer );
    createAccumulationBuffer();

    for ( int i : std::views::iota( 0, m_framesInFlight ) )
    {
        vk::DescriptorBufferInfo accelBufferInfo;
        accelBufferInfo.buffer = m_accBuffer;
        accelBufferInfo.offset = 0;
        accelBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet accelBufferWrite;
        accelBufferWrite.dstSet = m_rtDescriptorSets[i];
        accelBufferWrite.dstBinding = 5;
        accelBufferWrite.dstArrayElement = 0;
        accelBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        accelBufferWrite.descriptorCount = 1;
        accelBufferWrite.pBufferInfo = &accelBufferInfo;
        accelBufferWrite.pImageInfo = nullptr;        // Optional
        accelBufferWrite.pTexelBufferView = nullptr;  // Optional

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
            accelBufferWrite };

        vkUpdateDescriptorSets(
            m_device, 1, (VkWriteDescriptorSet*)writeDescriptorSets.data(), 0,
            nullptr );
    }
}

void RayTracer::updateTLAS( glm::mat4 model )
{
    m_asBuilder.updateTlas( m_tlas, m_blas, model );
}

void RayTracer::destroy()
{
    m_asBuilder.destroy();
    m_pipeline.destroy();
}