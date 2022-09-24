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
    const uint32_t numTriangles = numIndex / 3;

    //identity transform
    VkTransformMatrixKHR transformMatrix = { 1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f };

    std::vector<VkTransformMatrixKHR> mats;
    mats.push_back( transformMatrix );

    auto transformMatrixBuff =
        m_alloc->createAndStageBuffer( "BLAS Transform", mats );

    vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    vk::DeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( vertexBuffer );
    indexBufferDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( indexBuffer );
    transformBufferDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( transformMatrixBuff );

    //Description of the geometry, where the index and vertex data is
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

    //Description of what we're building (BLAS)
    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    geometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    geometryInfo.flags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    geometryInfo.geometryCount = 1;
    geometryInfo.pGeometries = &geometry;

    //get the size of everything
    vk::AccelerationStructureBuildSizesInfoKHR buildSizeInfo;

    // clang-format off
    AppState::instance().vkGetAccelerationStructureBuildSizesKHR(
        m_device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(&geometryInfo ),
        &numTriangles,
        reinterpret_cast<VkAccelerationStructureBuildSizesInfoKHR*>(&buildSizeInfo )
    );
    // clang-format on

    //allocate the blas memory
    vk::Buffer blas = m_alloc->createAccelStructureBuffer(
        name + " Buffer", buildSizeInfo.accelerationStructureSize );

    vk::AccelerationStructureCreateInfoKHR createInfo;
    createInfo.buffer = blas;
    createInfo.size = buildSizeInfo.accelerationStructureSize;
    createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    //The resulting handle
    vk::AccelerationStructureKHR handle;

    // clang-format off
    auto result = AppState::instance().vkCreateAccelerationStructureKHR(
        m_device,
        reinterpret_cast<VkAccelerationStructureCreateInfoKHR*>( &createInfo ),
        nullptr,
        reinterpret_cast<VkAccelerationStructureKHR*>( &handle ) 
    );
    // clang-format on

    checkSuccess( result );

    //allocate scratch buffer
    vk::Buffer blasScratch = m_alloc->createScratchBuffer(
        name + " Scratch", buildSizeInfo.buildScratchSize );

    auto blasScratchAddress = m_alloc->getDeviceAddress( blasScratch );

    //BLAS description
    vk::AccelerationStructureBuildGeometryInfoKHR asInfo;
    asInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    asInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    asInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    asInfo.dstAccelerationStructure = handle;
    asInfo.geometryCount = 1;
    asInfo.pGeometries = &geometry;
    asInfo.scratchData.deviceAddress = blasScratchAddress;

    //BLAS range description
    vk::AccelerationStructureBuildRangeInfoKHR asRangeInfo;
    asRangeInfo.primitiveCount = numTriangles;
    asRangeInfo.primitiveOffset = 0;
    asRangeInfo.firstVertex = 0;
    asRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
        accelerationBuildStructureRangeInfos = {
            reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR*>(
                &asRangeInfo ) };

    //Perform the BLAS build
    auto buffer = m_pool.beginOneTimeSubmit( name + " creation" );

    // clang-format off
    AppState::instance().vkCmdBuildAccelerationStructuresKHR(
        buffer, 
        1,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(&asInfo ),
        accelerationBuildStructureRangeInfos.data() 
    );
    // clang-format on

    m_pool.endOneTimeSubmit( buffer );

    //get the BLAS hardware address, needed to build TLAS later
    vk::AccelerationStructureDeviceAddressInfoKHR adressInfo{};
    adressInfo.accelerationStructure = handle;

    // clang-format off
    auto blasAddress =
        AppState::instance().vkGetAccelerationStructureDeviceAddressKHR(
            m_device,
            reinterpret_cast<VkAccelerationStructureDeviceAddressInfoKHR*>(&adressInfo ) 
    );
    // clang-format on

    m_structures.push_back( handle );
    m_addresses[handle] = blasAddress;
    DEBUG_NAME( handle, name );

    m_alloc->free( blasScratch );

    return handle;
}

//Build the TLAS, with one BLAS
vk::AccelerationStructureKHR ASBuilder::buildTlas(
    std::string name, vk::AccelerationStructureKHR blas )
{
    VkTransformMatrixKHR transformMatrix = { 1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f };

    //the BLAS instance that we're putting into the TLAS
    vk::AccelerationStructureInstanceKHR instance;
    instance.transform = transformMatrix;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = getAddress( blas );

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    instances.push_back( instance );

    VkBuffer instanceBuff =
        m_alloc->createAndStageBuffer( "TLAS Instance", instances );

    vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress;
    instanceDataDeviceAddress.deviceAddress =
        m_alloc->getDeviceAddress( instanceBuff );

    //geomery
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.geometryType = vk::GeometryTypeKHR::eInstances;
    geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
    geometry.geometry.instances.sType =
        vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
    geometry.geometry.instances.arrayOfPointers = false;
    geometry.geometry.instances.data = instanceDataDeviceAddress;

    //geometry info
    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    geometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    geometryInfo.flags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    geometryInfo.geometryCount = 1;
    geometryInfo.pGeometries = &geometry;

    uint32_t primitive_count = 1;

    vk::AccelerationStructureBuildSizesInfoKHR sizeInfo;

    //Get the required size

    // clang-format off
    AppState::instance().vkGetAccelerationStructureBuildSizesKHR(
        m_device, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>( &geometryInfo ),
        &primitive_count,
        reinterpret_cast<VkAccelerationStructureBuildSizesInfoKHR*>( &sizeInfo )
    );
    // clang-format on

    //allocate the tlas memory
    vk::Buffer tlas = m_alloc->createAccelStructureBuffer(
        name + " Buffer", sizeInfo.accelerationStructureSize );

    vk::AccelerationStructureCreateInfoKHR createInfo;
    createInfo.buffer = tlas;
    createInfo.size = sizeInfo.accelerationStructureSize;
    createInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;

    //the resulting handle (TLAS)
    vk::AccelerationStructureKHR handle;

    // clang-format off
    auto result = AppState::instance().vkCreateAccelerationStructureKHR(
        m_device,
        reinterpret_cast<VkAccelerationStructureCreateInfoKHR*>( &createInfo ),
        nullptr,
        reinterpret_cast<VkAccelerationStructureKHR*>( &handle ) 
    );
    // clang-format on

    checkSuccess( result );

    //allocate scratch buffer
    vk::Buffer tlasScratch = m_alloc->createScratchBuffer(
        name + " Scratch", sizeInfo.buildScratchSize );

    auto tlasScratchAddress = m_alloc->getDeviceAddress( tlasScratch );

    vk::AccelerationStructureBuildGeometryInfoKHR asInfo;
    asInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    asInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    asInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    asInfo.dstAccelerationStructure = handle;
    asInfo.geometryCount = 1;
    asInfo.pGeometries = &geometry;
    asInfo.scratchData.deviceAddress = tlasScratchAddress;

    vk::AccelerationStructureBuildRangeInfoKHR asRangeInfo;
    asRangeInfo.primitiveCount = 1;
    asRangeInfo.primitiveOffset = 0;
    asRangeInfo.firstVertex = 0;
    asRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
        accelerationBuildStructureRangeInfos = {
            reinterpret_cast<VkAccelerationStructureBuildRangeInfoKHR*>(
                &asRangeInfo ) };

    //Build the TLAS
    auto buffer = m_pool.beginOneTimeSubmit( name + " creation" );

    // clang-format off
    AppState::instance().vkCmdBuildAccelerationStructuresKHR(
        buffer,
        1,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>( &asInfo ),
        accelerationBuildStructureRangeInfos.data()
    );
    // clang-format on

    m_pool.endOneTimeSubmit( buffer );

    //get the TLAS hardware address
    vk::AccelerationStructureDeviceAddressInfoKHR adressInfo{};
    adressInfo.accelerationStructure = handle;

    // clang-format off
    auto tlasAddress =
        AppState::instance().vkGetAccelerationStructureDeviceAddressKHR(
            m_device,
            reinterpret_cast<VkAccelerationStructureDeviceAddressInfoKHR*>(&adressInfo ) 
    );
    // clang-format on

    m_structures.push_back( handle );
    m_addresses[handle] = tlasAddress;
    DEBUG_NAME( handle, name );

    m_alloc->free( tlasScratch );

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
