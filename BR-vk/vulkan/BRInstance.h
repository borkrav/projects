#pragma once

#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Instance
{
    friend class AppState;

   private:
    Instance();
    ~Instance();

    void create( bool enableValidationLayers );
    void createDebugMessenger();
    void destroy();
    VkInstance get();

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    bool m_enableValidationLayers;
};
}  // namespace BR
