#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

namespace BR
{
class Instance
{
   public:
    Instance();
    ~Instance();

    void create(bool enableValidationLayers);
    void createDebugMessenger();
    void destroy();
    VkInstance get();

   private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    bool m_enableValidationLayers;
};
}  // namespace BR
