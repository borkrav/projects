#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace BR
{
class Instance
{
    friend class AppState;

   private:
    Instance();
    ~Instance();
    Instance( const Instance& ) = delete;

    void create( bool enableValidationLayers );
    void createDebugMessenger();
    void destroy();

    vk::UniqueInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    bool m_enableValidationLayers;
};
}  // namespace BR
