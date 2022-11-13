#include <BRAppState.h>

#include <cassert>

using namespace BR;

AppState::AppState()
{
    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    m_window =
        glfwCreateWindow( m_iWidth, m_iHeight, "Vulkan", nullptr, nullptr );
    init( m_window, enableValidationLayers );
}

AppState::~AppState()
{
    m_swapchain.destroy();
    m_surface.destroy();
    m_instance.destroy();
    m_descMgr.destroy();
    m_syncMgr.destroy();
    m_memoryMgr.destroy();
}

void AppState::init( GLFWwindow* window, bool debug )
{
    m_window = window;

    m_instance.create( debug );
    m_surface.create( window, m_instance.m_instance.get() );

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    };

    m_device.create( "GPU", deviceExtensions, m_instance.m_instance.get(),
                     m_surface.m_surface );

#ifdef NDEBUG
#else
    m_debug.create( " RTX 3080 ", m_device.m_logicalDevice.get() );
#endif

    m_swapchain.create( "Swapchain", window, m_device.m_physicalDevice,
                        m_device.m_logicalDevice.get(), m_device.m_index,
                        m_surface.m_surface );

#ifdef NDEBUG
#else
    m_debug.setName( m_swapchain.m_swapChain, "Swapchain" );
#endif

    m_swapchain.createImageViews( "Swapchain image view " );

#ifdef NDEBUG
#else
    for ( int i = 0; i < m_swapchain.m_swapChainImageViews.size(); ++i )
        m_debug.setName( m_swapchain.m_swapChainImageViews[i],
                         "Swapchain image view " + i );
#endif

    initRT();
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

void AppState::recreateSwapchain()
{
    //destroy old swapchain
    m_swapchain.destroy();

    //create new swapchain
    m_swapchain.create( "Swapchain", m_window, m_device.m_physicalDevice,
                        m_device.m_logicalDevice.get(), m_device.m_index,
                        m_surface.m_surface );
    m_swapchain.createImageViews( "Swapchain image view " );
}

vk::Instance AppState::getInstance()
{
    return m_instance.m_instance.get();
}

vk::PhysicalDevice AppState::getPhysicalDevice()
{
    return m_device.m_physicalDevice;
}

vk::Device AppState::getLogicalDevice()
{
    return m_device.m_logicalDevice.get();
}

vk::Queue AppState::getGraphicsQueue()
{
    return m_device.m_graphicsQueue;
}

int AppState::getFamilyIndex()
{
    return m_device.m_index;
}

vk::SurfaceKHR AppState::getSurface()
{
    return m_surface.m_surface;
}

vk::SwapchainKHR AppState::getSwapchain()
{
    return m_swapchain.m_swapChain;
}

vk::Image AppState::getSwapchainImage( int index )
{
    assert( index >= 0 && index < m_swapchain.m_swapChainImages.size() );

    return m_swapchain.m_swapChainImages[index];
}

vk::Format AppState::getSwapchainFormat()
{
    return m_swapchain.m_swapChainFormat;
}

vk::Extent2D& AppState::getSwapchainExtent()
{
    return m_swapchain.m_swapChainExtent;
}

std::vector<vk::ImageView>& AppState::getImageViews()
{
    return m_swapchain.m_swapChainImageViews;
}

BR::DescMgr& AppState::getDescMgr()
{
    return m_descMgr;
}

BR::SyncMgr& AppState::getSyncMgr()
{
    return m_syncMgr;
}

BR::MemoryMgr& AppState::getMemoryMgr()
{
    return m_memoryMgr;
}

GLFWwindow* AppState::getWindow()
{
    return m_window;
}

void AppState::takeScreenshot( CommandPool& pool, int frame )
{
    m_swapchain.takeScreenshot( pool, frame );
}

#ifdef NDEBUG
#else
BR::Debug AppState::getDebug()
{
    return m_debug;
}
#endif