#include <BRASBuilder.h>
#include <BRAppState.h>

#include <cassert>

using namespace BR;

ASBuilder::ASBuilder() : m_alloc( nullptr )
{
}

ASBuilder::~ASBuilder()
{
    assert( m_structures.empty() );
}

void ASBuilder::create( BufferAllocator& mainAlloc )
{
    m_device = AppState::instance().getLogicalDevice();
    m_alloc = &mainAlloc;
    m_pool.create( "ASBuilder Command Pool",
                   vk::CommandPoolCreateFlagBits::eTransient );
}

vk::AccelerationStructureKHR ASBuilder::buildBlas( std::string name,
                                                   vk::Buffer vertexBuffer,
                                                   vk::Buffer indexBuffer,
                                                   int maxVertex, int numIndex )
{
    //TODO: clean this up, really confusing


    //for now, identity transform
    VkTransformMatrixKHR transformMatrix = { 1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f };

    std::vector<VkTransformMatrixKHR> test;
    test.push_back( transformMatrix );

    auto transformMatrixBuff = m_alloc->createAndStageBuffer(
        "BLAS Transform", test, vk::BufferUsageFlagBits::eVertexBuffer );

    vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    vk::DeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( vertexBuffer );
    indexBufferDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( indexBuffer );
    transformBufferDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( transformMatrixBuff );

    //The connections to the geometry buffers
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
    geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
    geometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
    geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    geometry.geometry.triangles.maxVertex = maxVertex;
    geometry.geometry.triangles.vertexStride = sizeof( Pipeline::Vertex );
    geometry.geometry.triangles.indexType = vk::IndexType::eUint16;
    geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    geometry.geometry.triangles.transformData.deviceAddress = 0;
    geometry.geometry.triangles.transformData.hostAddress = nullptr;
    geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

    //What we're trying to build
    vk::AccelerationStructureBuildGeometryInfoKHR sizeInfo;
    sizeInfo.sType =
        vk::StructureType::eAccelerationStructureBuildGeometryInfoKHR;
    sizeInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    sizeInfo.flags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    sizeInfo.geometryCount = 1;
    sizeInfo.pGeometries = &geometry;

    const uint32_t numTriangles = numIndex / 3;

    //get the size of everything
    VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{};
    buildSizeInfo.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    AppState::instance().vkGetAccelerationStructureBuildSizesKHR(
        m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(
            &sizeInfo ),
        &numTriangles, &buildSizeInfo );

    //allocate the blas memory
    vk::Buffer blas = m_alloc->createAccelStructureBuffer(
        name +" Buffer", buildSizeInfo.accelerationStructureSize );


    vk::AccelerationStructureCreateInfoKHR createInfo;
    createInfo.buffer = blas;
    createInfo.size = buildSizeInfo.accelerationStructureSize;
    createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    //The resulting handle
    VkAccelerationStructureKHR handle;

    AppState::instance().vkCreateAccelerationStructureKHR(
        m_device,
        reinterpret_cast<VkAccelerationStructureCreateInfoKHR*>( &createInfo ),
        nullptr, &handle );

    //allocate scratch buffer
    vk::Buffer blasScratch = m_alloc->createScratchBuffer(
        name +" Scratch", buildSizeInfo.buildScratchSize );

    auto blasScratchAddress = m_alloc->getDeviceAddress( blasScratch );

    // build the actual BLAS on the GPU
    vk::AccelerationStructureBuildGeometryInfoKHR
        accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.type =
        vk::AccelerationStructureTypeKHR::eBottomLevel;
    accelerationBuildGeometryInfo.flags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    accelerationBuildGeometryInfo.mode =
        vk::BuildAccelerationStructureModeKHR::eBuild;
    accelerationBuildGeometryInfo.dstAccelerationStructure = handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &geometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress =
        blasScratchAddress;

    vk::AccelerationStructureBuildRangeInfoKHR
        accelerationStructureBuildRangeInfo;
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
        accelerationBuildStructureRangeInfos = {
            reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR*>(
                &accelerationStructureBuildRangeInfo ) };

    //Finally build the BLAS...
    auto buffer = m_pool.beginOneTimeSubmit( name +" creation" );
    AppState::instance().vkCmdBuildAccelerationStructuresKHR(
        buffer, 1,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(
            &accelerationBuildGeometryInfo ),
        accelerationBuildStructureRangeInfos.data() );
    m_pool.endOneTimeSubmit( buffer );

    //get the BLAS hardware address, needed to build TLAS later
    VkAccelerationStructureDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    bufferDeviceAI.accelerationStructure = handle;
    auto blasAddress =
        AppState::instance().vkGetAccelerationStructureDeviceAddressKHR(
            m_device, &bufferDeviceAI );

    m_structures.push_back( handle );
    m_addresses[handle] = blasAddress;
    DEBUG_NAME( handle, name );

    m_alloc->free( blasScratch );

    return handle;
}

uint64_t ASBuilder::getAddress( vk::AccelerationStructureKHR structure )
{
    auto it = m_addresses.find( structure );
    assert( it != m_addresses.end() );
    return it->second;
}

void ASBuilder::destroy()
{
    m_pool.destroy();

    for ( auto handle : m_structures )
        AppState::instance().vkDestroyAccelerationStructureKHR(
            m_device, handle, nullptr );

    m_addresses.clear();
    m_structures.clear();
}
