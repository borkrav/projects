#pragma once

#include <BRDevice.h>
#include <BRInstance.h>
#include <BRSurface.h>
#include <BRSwapchain.h>

#include <vulkan/vulkan_handles.hpp>

namespace BR
{

//Singleton class defining the Vulkan application state, containing:

//1 VkInstance
//1 VkPhysicalDevice/VkDevice
//1 VkSurfaceKHR
//1 VkSwapchainKHR

// This can be extended to support multiple devices, multiple Surfaces + Swapchains
// For now, this is not supported, only supporting 1 GPU rendering to 1 window

class AppState
{
   public:
    static AppState& instance()
    {
        static AppState instance;
        return instance;
    }

    AppState( AppState const& ) = delete;
    void operator=( AppState const& ) = delete;

    void init( GLFWwindow* window );

    void recreateSwapchain();

    VkInstance getInstance();

    VkPhysicalDevice getPhysicalDevice();
    VkDevice getLogicalDevice();
    VkQueue getGraphicsQueue();
    int getFamilyIndex();

    VkSurfaceKHR getSurface();

    VkSwapchainKHR getSwapchain();
    VkFormat getSwapchainFormat();
    VkExtent2D& getSwapchainExtent();
    std::vector<VkImageView>& getImageViews();

   private:
    AppState();
    ~AppState();

    GLFWwindow* m_window;

    bool created;

    BR::Instance m_instance;
    BR::Device m_device;
    BR::Surface m_surface;
    BR::Swapchain m_swapchain;
};
}  // namespace BR