#include <BRAppState.h>
#include <BRDevice.h>
#include <Util.h>

#include <cassert>

using namespace BR;

Device::Device()
    : m_physicalDevice( VK_NULL_HANDLE ),
      m_logicalDevice( VK_NULL_HANDLE ),
      m_graphicsQueue( VK_NULL_HANDLE ),
      m_index( 0 )
{
}

//TODO surface should be it's own object
void Device::create( std::string name,
                     const std::vector<const char*>& deviceExtensions )
{
    uint32_t deviceCount = 0;

    auto devices =
        AppState::instance().getInstance().enumeratePhysicalDevices();

    vkEnumeratePhysicalDevices( AppState::instance().getInstance(),
                                &deviceCount, nullptr );

    assert( !devices.empty() );

    // pick the first GPU in the system
    m_physicalDevice = devices[0];

    /*
    * Vulkan has concept of physical and logical device
    * A VkPhysicalDevice is the hardware - there's only one
    * A VkDevice is an instance of the hardware, for this application - can have more than one
    */

    // get the queue families
    uint32_t queueFamilyCount = 0;

    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

    // ensure one queue has graphics bit
    int graphicsFamilyIndex = -1;

    int i = 0;
    for ( const auto& family : queueFamilies )
    {
        if ( family.queueFlags & vk::QueueFlagBits::eGraphics )
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

    float priority = 1.0f;
    auto queueCreateInfo = vk::DeviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        static_cast<uint32_t>( graphicsFamilyIndex ), 1, &priority );

    auto createInfo =
        vk::DeviceCreateInfo( vk::DeviceCreateFlags(), 1, &queueCreateInfo );

    // enable swapchain
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>( deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // assuming the graphics queue also has presentation support
    bool presentSupport = m_physicalDevice.getSurfaceSupportKHR(
        graphicsFamilyIndex, AppState::instance().getSurface() );
    assert( presentSupport );

    try
    {
        m_logicalDevice = m_physicalDevice.createDeviceUnique( createInfo );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create logical device!" );
    }

    m_graphicsQueue = m_logicalDevice->getQueue( graphicsFamilyIndex, 0 );

    auto properties = m_physicalDevice.getProperties();

    printf( "\nSelected Physical Device\n" );
    printf( "\nName:\n" );
    printf( "\t%s\n", properties.deviceName );
    printf( "\nQueue Families:\n" );

    int famCounter = 0;
    for ( const auto& family : queueFamilies )
    {
        printf( "\tQueue number: %s\n",
                std::to_string( family.queueCount ).c_str() );
        printf(
            "\tQueue flags: %s%s%s%s%s \n",
            family.queueFlags & vk::QueueFlagBits::eGraphics ? "Graphics " : "",
            family.queueFlags & vk::QueueFlagBits::eCompute ? "Compute " : "",
            family.queueFlags & vk::QueueFlagBits::eTransfer ? "Transfer " : "",
            family.queueFlags & vk::QueueFlagBits::eSparseBinding
                ? "SparseBinding "
                : "",
            family.queueFlags & vk::QueueFlagBits::eProtected ? "Protected "
                                                              : "" );

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( m_physicalDevice, famCounter++,
                                              AppState::instance().getSurface(),
                                              &presentSupport );

        printf( "\tPresentation Support: %d\n\n", presentSupport );
    }
    printf( "\nDevice Extensions:\n" );
    for ( auto extension : deviceExtensions )
        printf( "\t%s\n", extension );
    printf( "\nGot Device queue from family with index: %d\n",
            graphicsFamilyIndex );
}
