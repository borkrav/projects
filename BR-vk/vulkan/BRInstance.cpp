#include <BRInstance.h>
#include <BRUtil.h>
#include <BRUtil.h>

using namespace BR;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation" };

bool checkValidationLayerSupport()
{
    auto layers = vk::enumerateInstanceLayerProperties();

    for ( const char* layerName : validationLayers )
    {
        bool layerFound = false;

        for ( const auto& layerProperties : layers )
        {
            if ( strcmp( layerName, layerProperties.layerName ) == 0 )
            {
                layerFound = true;
                break;
            }
        }

        if ( !layerFound )
        {
            return false;
        }
    }
    return true;
}

std::vector<const char*> getExtensions( bool enableValidationLayers )
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    std::vector<const char*> result( glfwExtensions,
                                     glfwExtensions + glfwExtensionCount );

    if ( enableValidationLayers )
        result.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    return result;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger )
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
    printf( "\n --> validation layer: %s\n", pCallbackData->pMessage );
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    vk::DebugUtilsMessengerCreateInfoEXT& createInfo )
{
    createInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;

    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                             vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                             vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}

void DestroyDebugUtilsMessengerEXT( VkInstance instance,
                                    VkDebugUtilsMessengerEXT debugMessenger,
                                    const VkAllocationCallbacks* pAllocator )
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( instance, debugMessenger, pAllocator );
    }
}

Instance::Instance() : m_instance( nullptr ), m_enableValidationLayers( false )
{
}

Instance::~Instance()
{
}

void Instance::create( bool enableValidationLayers )
{
    m_enableValidationLayers = enableValidationLayers;

    if ( m_enableValidationLayers )
        assert( checkValidationLayerSupport() );

    /*
    * Allows application to "introduce itself" to the driver.
    * Drivers implement code paths/optimizations for popular
        applications/engines In older APIs, drivers would use huertics to detect
        the application, this is cleaner.
    */

    auto appInfo = vk::ApplicationInfo(
        "Hello Triangle", VK_MAKE_VERSION( 1, 0, 0 ), "No Engine",
        VK_MAKE_VERSION( 1, 0, 0 ), VK_API_VERSION_1_0 );

    /*
    * Structure for creation of vulkan instance, specifies layers and extensions
    * Layers insert themselves between the Application and the Driver, this is useful
        for debugging and validation
    * Vulkan calls form a Call Chain
        VkFunction -> Loader Trampoline -> Layer A -> Layer B -> Layer C -> Driver
        Layer can chose to ignore functions, don't have to implement for everything
        Layer can check the function parameters, then call the function
        Works as a chain of function pointers
    * Extensions extend vulkan functionality, different hardware can provide different
        extensions. The base vulkan spec provides very little, so you must ask for
        features that are needed 
    */

    vk::InstanceCreateInfo createInfo;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;

    createInfo.pApplicationInfo = &appInfo;

    if ( m_enableValidationLayers )
    {
        /*
        * Allows debug messenging for creation/destruction of the instance
        * The debug messenger object does not exist yet, it's created in 
            VulkanEngine::setupDebugMessenger()
        */
        createInfo.enabledLayerCount =
            static_cast<uint32_t>( validationLayers.size() );
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo( debugCreateInfo );
        createInfo.pNext =
            (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    auto extensions = getExtensions( m_enableValidationLayers );
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>( extensions.size() );
    createInfo.ppEnabledExtensionNames = extensions.data();

    /*
    * Vulkan application state is stored inside an instance
    * Creating an instance does the following:
        Initializes the vulkan loader
        Loader loads and initializes the vulkan driver
        Loader optionally loads any layers
    * The application is linked against vulkan-1.dll, which is the loader
    */

    try
    {
        m_instance = vk::createInstanceUnique( createInfo, nullptr );
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create instance!" );
    }

    printf( "\nCreated Vulkan instance\n" );
    printf( "\nLayers:\n" );
    for ( auto layer : validationLayers )
        printf( "\t%s\n", layer );
    printf( "\nExtensions:\n" );
    for ( auto extension : extensions )
        printf( "\t%s\n", extension );

    createDebugMessenger();
}

void Instance::createDebugMessenger()
{
    /*
    * Creates a debug messenger object, which triggers a callback,
        debug messenger. Validation layers call this messenger 
    */

    if ( !m_enableValidationLayers )
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo( createInfo );

    auto result = CreateDebugUtilsMessengerEXT(
        *m_instance,
        reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(
            &createInfo ),
        nullptr, &m_debugMessenger );

    CHECK_SUCCESS( result );
}

void Instance::destroy()
{
    if ( m_enableValidationLayers )
        DestroyDebugUtilsMessengerEXT( *m_instance, m_debugMessenger, nullptr );
}
