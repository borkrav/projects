#include <BRAppState.h>

using namespace BR;

void Debug::create( std::string name )
{
    m_device = AppState::instance().getLogicalDevice();
    setName( (uint64_t)(VkDevice)m_device, name, VK_OBJECT_TYPE_DEVICE );
}

void setDebugUtilsObjectNameEXT( vk::Device device,
                                 VkDebugUtilsObjectNameInfoEXT* s )
{
    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(
        device, "vkSetDebugUtilsObjectNameEXT" );
    if ( func != nullptr )
    {
        func( device, s );
    }
}

void Debug::setName( const uint64_t object, const std::string& name,
                     VkObjectType t )
{
    assert( m_device );

    VkDebugUtilsObjectNameInfoEXT s{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, t, object,
        name.c_str() };
    setDebugUtilsObjectNameEXT( m_device, &s );
}
