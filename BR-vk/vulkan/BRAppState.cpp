#include <BRAppState.h>

#include <cassert>

using namespace BR;

AppState::AppState() : created( false )
{
}

AppState::~AppState()
{
    assert( created );

    m_swapchain.destroy();
    m_surface.destroy();
    m_instance.destroy();
}

void AppState::getFunctionPointers()
{
    //Ray Tracing function pointers

    auto device = m_device.m_logicalDevice.get();

    vkGetBufferDeviceAddressKHR =
        reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
            vkGetDeviceProcAddr( device, "vkGetBufferDeviceAddressKHR" ) );
    vkCmdBuildAccelerationStructuresKHR =
        reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr( device,
                                 "vkCmdBuildAccelerationStructuresKHR" ) );
    vkBuildAccelerationStructuresKHR =
        reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr( device, "vkBuildAccelerationStructuresKHR" ) );
    vkCreateAccelerationStructureKHR =
        reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
            vkGetDeviceProcAddr( device, "vkCreateAccelerationStructureKHR" ) );
    vkDestroyAccelerationStructureKHR =
        reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
            vkGetDeviceProcAddr( device,
                                 "vkDestroyAccelerationStructureKHR" ) );
    vkGetAccelerationStructureBuildSizesKHR =
        reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
            vkGetDeviceProcAddr( device,
                                 "vkGetAccelerationStructureBuildSizesKHR" ) );
    vkGetAccelerationStructureDeviceAddressKHR =
        reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            vkGetDeviceProcAddr(
                device, "vkGetAccelerationStructureDeviceAddressKHR" ) );
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
        vkGetDeviceProcAddr( device, "vkCmdTraceRaysKHR" ) );
    vkGetRayTracingShaderGroupHandlesKHR =
        reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
            vkGetDeviceProcAddr( device,
                                 "vkGetRayTracingShaderGroupHandlesKHR" ) );
    vkCreateRayTracingPipelinesKHR =
        reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
            vkGetDeviceProcAddr( device, "vkCreateRayTracingPipelinesKHR" ) );
}

void AppState::initRT()
{
    getFunctionPointers();

    //get all the properties, these are needed to set up structures
    auto gpu = m_device.m_physicalDevice;

    rayTracingPipelineProperties.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2( gpu, &deviceProperties2 );

    accelerationStructureFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &accelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2( gpu, &deviceFeatures2 );
}

void AppState::init( GLFWwindow* window, bool debug )
{
    if ( created )
        return;

    created = true;

    m_window = window;

    m_instance.create( debug );
    m_surface.create( window );

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, };

    m_device.create( "GPU", deviceExtensions );

#ifdef NDEBUG
#else
    m_debug.create( " RTX 3080 " );
#endif

    m_swapchain.create( "Swapchain", window );
    m_swapchain.createImageViews( "Swapchain image view " );

    initRT();
}

void AppState::recreateSwapchain()
{
    //destroy old swapchain
    m_swapchain.destroy();

    //create new swapchain
    m_swapchain.create( "Swapchain", m_window );
    m_swapchain.createImageViews( "Swapchain image view " );
}

vk::Instance AppState::getInstance()
{
    assert( created );
    return m_instance.m_instance.get();
}

vk::PhysicalDevice AppState::getPhysicalDevice()
{
    assert( created );
    return m_device.m_physicalDevice;
}

vk::Device AppState::getLogicalDevice()
{
    assert( created );
    return m_device.m_logicalDevice.get();
}

vk::Queue AppState::getGraphicsQueue()
{
    assert( created );
    return m_device.m_graphicsQueue;
}

int AppState::getFamilyIndex()
{
    assert( created );
    return m_device.m_index;
}

vk::SurfaceKHR AppState::getSurface()
{
    assert( created );
    return m_surface.m_surface;
}

vk::SwapchainKHR AppState::getSwapchain()
{
    assert( created );
    return m_swapchain.m_swapChain;
}

vk::Image AppState::getSwapchainImage( int index )
{
    assert( created );
    assert( index >= 0 && index < m_swapchain.m_swapChainImages.size() );

    return m_swapchain.m_swapChainImages[index];
}

vk::Format AppState::getSwapchainFormat()
{
    assert( created );
    return m_swapchain.m_swapChainFormat;
}

vk::Extent2D& AppState::getSwapchainExtent()
{
    assert( created );
    return m_swapchain.m_swapChainExtent;
}

std::vector<vk::ImageView>& AppState::getImageViews()
{
    assert( created );
    return m_swapchain.m_swapChainImageViews;
}

#ifdef NDEBUG
#else
BR::Debug AppState::getDebug()
{
    return m_debug;
}
#endif