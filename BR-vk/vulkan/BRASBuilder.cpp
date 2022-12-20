#include <BRASBuilder.h>
#include <BRAppState.h>
#include <random>
#include <ranges>

#include <cassert>

using namespace BR;

ASBuilder::ASBuilder() : m_alloc( AppState::instance().getMemoryMgr() )
{
}

ASBuilder::~ASBuilder()
{
    assert( m_structures.empty() );
}

void ASBuilder::create()
{
    m_device = AppState::instance().getLogicalDevice();
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

    auto bufferSize = sizeof( mats[0] ) * mats.size();
    auto transformMatrixBuff = m_alloc.createDeviceBuffer(
        "BLAS Transform", bufferSize, mats.data(), false,
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::
                eAccelerationStructureBuildInputReadOnlyKHR );

    vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    vk::DeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress =
        m_alloc.getDeviceAddress( vertexBuffer );
    indexBufferDeviceAddress.deviceAddress =
        m_alloc.getDeviceAddress( indexBuffer );
    transformBufferDeviceAddress.deviceAddress =
        m_alloc.getDeviceAddress( transformMatrixBuff );

    //Description of the geometry, where the index and vertex data is
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
    geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
    geometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
    geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    geometry.geometry.triangles.maxVertex = maxVertex;
    geometry.geometry.triangles.vertexStride = sizeof( glm::vec4 );
    geometry.geometry.triangles.indexType = vk::IndexType::eUint32;
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
    vk::Buffer blas = m_alloc.createDeviceBuffer(
        name + " Buffer", buildSizeInfo.accelerationStructureSize, nullptr,
        false,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress );

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
    vk::Buffer blasScratch = m_alloc.createDeviceBuffer(
        name + " Scratch", buildSizeInfo.buildScratchSize, nullptr, false,
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress );

    auto blasScratchAddress = m_alloc.getDeviceAddress( blasScratch );

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

    m_alloc.free( blasScratch );

    return handle;
}

//Build the TLAS, with one BLAS
vk::AccelerationStructureKHR ASBuilder::buildTlas(
    std::string name, vk::AccelerationStructureKHR blas )
{
    std::vector<vk::AccelerationStructureInstanceKHR> m_instances;

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 100000);

    for ( int i : std::views::iota( 0, 152 ) )
    {
        glm::mat4 transform( 1 );

        float rand1 = (float)distrib( gen ) / 100000;
        float rand2 = (float)distrib( gen ) / 100000;
        float rand3 = (float)distrib( gen ) / 100000;

        VkTransformMatrixKHR transformMatrix = {
            rand1 + 0.5, 0.0f,        0.0f,        15 - ( rand1 * 30 ),
            0.0f,        rand1 + 0.5, 0.0f,        15 - ( rand2 * 30 ),
            0.0f,        0.0f,        rand1 + 0.5, 15 - ( rand3 * 30 ) };

        //the BLAS instance that we're putting into the TLAS
        vk::AccelerationStructureInstanceKHR instance;
        instance.transform = transformMatrix;
        instance.instanceCustomIndex = 0;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags =
            VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = getAddress( blas );

        m_instances.push_back( instance );
    }

    vk::BufferUsageFlags flags =
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

    m_instanceBuff = m_alloc.createDeviceBuffer(
        name + " instance buffer",
        m_instances.size()*sizeof( vk::AccelerationStructureInstanceKHR ), m_instances.data(), true,
        flags );

    vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress;
    instanceDataDeviceAddress.deviceAddress =
        m_alloc.getDeviceAddress( m_instanceBuff );

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
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
        vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
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
    vk::Buffer tlas = m_alloc.createDeviceBuffer(
        name + " Buffer", sizeInfo.accelerationStructureSize, nullptr, false,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress );

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
    m_tlasScratch = m_alloc.createDeviceBuffer(
        name + " Scratch", sizeInfo.buildScratchSize, nullptr, false,
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress );

    auto tlasScratchAddress = m_alloc.getDeviceAddress( m_tlasScratch );

    vk::AccelerationStructureBuildGeometryInfoKHR asInfo;
    asInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    asInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                   vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    asInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    asInfo.dstAccelerationStructure = handle;
    asInfo.geometryCount = 1;
    asInfo.pGeometries = &geometry;
    asInfo.scratchData.deviceAddress = tlasScratchAddress;

    vk::AccelerationStructureBuildRangeInfoKHR asRangeInfo;
    asRangeInfo.primitiveCount = m_instances.size();
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

    return handle;
}

void ASBuilder::updateTlas( vk::AccelerationStructureKHR tlas,
                            vk::AccelerationStructureKHR blas, glm::mat4 mat )
{
    VkTransformMatrixKHR transformMatrix = {
        mat[0][0], mat[1][0], mat[2][0], mat[3][0], mat[0][1], mat[1][1],
        mat[2][1], mat[3][1], mat[0][2], mat[1][2], mat[2][2], mat[3][2] };

    //the BLAS instance that we're putting into the TLAS
    vk::AccelerationStructureInstanceKHR instance;
    instance.transform = transformMatrix;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR |
                     VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    instance.accelerationStructureReference = getAddress( blas );

    m_alloc.updateVisibleBuffer(
        m_instanceBuff, sizeof( vk::AccelerationStructureInstanceKHR ),
        &instance );

    vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress;
    instanceDataDeviceAddress.deviceAddress =
        m_alloc.getDeviceAddress( m_instanceBuff );

    //geomery
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.geometryType = vk::GeometryTypeKHR::eInstances;
    geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
    geometry.geometry.instances.sType =
        vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
    geometry.geometry.instances.arrayOfPointers = false;
    geometry.geometry.instances.data = instanceDataDeviceAddress;

    auto tlasScratchAddress = m_alloc.getDeviceAddress( m_tlasScratch );

    vk::AccelerationStructureBuildGeometryInfoKHR asInfo;
    asInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    asInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                   vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    asInfo.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
    asInfo.dstAccelerationStructure = tlas;
    asInfo.srcAccelerationStructure = tlas;
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

    //Update the TLAS
    auto buffer = m_pool.beginOneTimeSubmit( "TLAS Update" );

    // clang-format off
    AppState::instance().vkCmdBuildAccelerationStructuresKHR(
        buffer,
        1,
        reinterpret_cast<VkAccelerationStructureBuildGeometryInfoKHR*>( &asInfo ),
        accelerationBuildStructureRangeInfos.data()
    );
    // clang-format on

    m_pool.endOneTimeSubmit( buffer );
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
    m_alloc.free( m_tlasScratch );
}
