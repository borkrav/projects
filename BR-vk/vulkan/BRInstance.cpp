#include <BRInstance.h>

#include <Util.h>

using namespace BR;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation" };

bool checkValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    for ( const char* layerName : validationLayers )
    {
        bool layerFound = false;

        for ( const auto& layerProperties : availableLayers )
        {
            if ( std::string( layerName ) ==
                 std::string( layerProperties.layerName ) )
            {
                layerFound = true;
                break;
            }
        }
        if ( !layerFound )
            return false;
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
    printf( "validation layer: %s\n", pCallbackData->pMessage );
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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



Instance::Instance() : m_instance( VK_NULL_HANDLE ), m_enableValidationLayers( false )
{
}

Instance::~Instance()
{
    assert( m_instance == VK_NULL_HANDLE );
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
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion = VK_API_VERSION_1_0;

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

    VkInstanceCreateInfo createInfo{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
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
            (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
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

    VkResult result = vkCreateInstance( &createInfo, nullptr, &m_instance );

    checkSuccess( result );

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

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo( createInfo );

    VkResult result = CreateDebugUtilsMessengerEXT(
        m_instance, &createInfo, nullptr, &m_debugMessenger );

    checkSuccess( result );
}

void Instance::destroy()
{
    if ( m_enableValidationLayers )
        DestroyDebugUtilsMessengerEXT( m_instance, m_debugMessenger, nullptr );

    vkDestroyInstance( m_instance, nullptr );
    m_instance = VK_NULL_HANDLE;
}

VkInstance Instance::get()
{
    assert( m_instance != VK_NULL_HANDLE );
    return m_instance;
}
