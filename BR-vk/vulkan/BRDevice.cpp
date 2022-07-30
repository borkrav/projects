#include "BRDevice.h"
#include "Util.h"
#include <cassert>

using namespace BR;

Device::Device()
    : m_physicalDevice( VK_NULL_HANDLE ),
      m_logicalDevice( VK_NULL_HANDLE ),
      m_graphicsQueue( VK_NULL_HANDLE ),
      m_index( 0 )
{
}

Device::~Device()
{
    assert( m_logicalDevice == VK_NULL_HANDLE );
}

//TODO surface should be it's own object
void Device::create(Instance &instance, const std::vector<const char*>&deviceExtensions, VkSurfaceKHR surface )
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( instance.get(), &deviceCount, nullptr );

    assert( deviceCount != 0 );

    std::vector<VkPhysicalDevice> devices( deviceCount );
    vkEnumeratePhysicalDevices( instance.get(), &deviceCount, devices.data() );

    // pick the first GPU in the system
    m_physicalDevice = devices[0];


    /*
    * Vulkan has concept of physical and logical device
    * A VkPhysicalDevice is the hardware - there's only one
    * A VkDevice is an instance of the hardware, for this application - can have more than one
    */

    // get the queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( m_physicalDevice, &queueFamilyCount,
                                              nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( m_physicalDevice, &queueFamilyCount,
                                              queueFamilies.data() );

    // ensure one queue has graphics bit
    int graphicsFamilyIndex = -1;

    int i = 0;
    for ( const auto& family : queueFamilies )
    {
        if ( family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            graphicsFamilyIndex = i++;
    }

    assert( graphicsFamilyIndex != -1 );

    m_index = graphicsFamilyIndex;

    /*
        Queue Families for the RTX 3080:
            Queue number: 16
            Queue flags: Graphics Compute Transfer SparseBinding
            Queue number: 2
            Queue flags: Transfer SparseBinding
            Queue number: 8
            Queue flags: Compute Transfer SparseBinding

    * 3 types of families, each supporting those usages
    * Can have up to Queue Number of queues for each type
    * In other words, I can have 16 graphics queues
    
    * Queue executes command buffers (could be more than one)
    * Work can be submitted to a queue from one thread at a time
    * Different threads can submit work to different queues in parallel

    * Multiple Queues may correspond to distinct hardware queues, or may all go to
        the same hardware-side queue. The hardware-level queue parallelism is not 
        exposed to Vulkan, this is hardware dependant
    */

    //Request for 1 queue from the graphics queue family
    VkDeviceQueueCreateInfo queueCreateInfo{};
    float priority = 1.0f;
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    // enable swapchain
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>( deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // assuming the graphics queue also has presentation support
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR( m_physicalDevice, graphicsFamilyIndex, surface,
                                          &presentSupport );
    assert( presentSupport );

    VkResult result =
        vkCreateDevice( m_physicalDevice, &createInfo, nullptr, &m_logicalDevice );

    checkSuccess( result );

    vkGetDeviceQueue( m_logicalDevice, graphicsFamilyIndex, 0, &m_graphicsQueue );

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties( m_physicalDevice, &properties );

    printf( "\nSelected Physical Device\n" );
    printf( "\nName:\n" );
    printf( "\t%s\n", properties.deviceName );
    printf( "\nQueue Families:\n" );

    int famCounter = 0;
    for ( const auto& family : queueFamilies )
    {
        printf( "\tQueue number: %s\n", std::to_string( family.queueCount ).c_str() );
        printf( "\tQueue flags: %s%s%s%s%s \n",
                family.queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Graphics " : "",
                family.queueFlags & VK_QUEUE_COMPUTE_BIT ? "Compute " : "",
                family.queueFlags & VK_QUEUE_TRANSFER_BIT ? "Transfer " : "",
                family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SparseBinding " : "",
                family.queueFlags & VK_QUEUE_PROTECTED_BIT ? "Protected " : "" );

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( m_physicalDevice, famCounter++,
                                              surface, &presentSupport );

        printf( "\tPresentation Support: %d\n\n", presentSupport );
    }
    printf( "\nDevice Extensions:\n" );
    for ( auto extension : deviceExtensions )
        printf( "\t%s\n", extension );
    printf( "\nGot Device queue from family with index: %d\n", graphicsFamilyIndex );
}


void Device::destroy()
{
    vkDestroyDevice( m_logicalDevice, nullptr );
    m_logicalDevice = VK_NULL_HANDLE;
}

VkPhysicalDevice Device::getPhysicaDevice()
{
    assert( m_physicalDevice != VK_NULL_HANDLE );
    return m_physicalDevice;
}

VkDevice Device::getLogicalDevice()
{
    assert( m_logicalDevice != VK_NULL_HANDLE );
    return m_logicalDevice;
}

int Device::getFamilyIndex()
{
    assert( m_physicalDevice != VK_NULL_HANDLE );
    return m_index;
}

VkQueue Device::getGraphicsQueue()
{
    assert( m_graphicsQueue != VK_NULL_HANDLE );
    return m_graphicsQueue;
}