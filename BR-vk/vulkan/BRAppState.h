#pragma once

#include <BRDebug.h>
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

    void init( GLFWwindow* window, bool debug );

    void recreateSwapchain();

    vk::Instance getInstance();

    vk::PhysicalDevice getPhysicalDevice();
    vk::Device getLogicalDevice();
    vk::Queue getGraphicsQueue();
    int getFamilyIndex();
    vk::SurfaceKHR getSurface();
    vk::SwapchainKHR getSwapchain();
    vk::Format getSwapchainFormat();
    vk::Extent2D& getSwapchainExtent();
    std::vector<vk::ImageView>& getImageViews();

#ifdef NDEBUG
#else
    BR::Debug getDebug();
#endif

   private:
    AppState();
    ~AppState();

    GLFWwindow* m_window;

    bool created;

    BR::Instance m_instance;
    BR::Device m_device;
    BR::Surface m_surface;
    BR::Swapchain m_swapchain;

#ifdef NDEBUG
#else
    BR::Debug m_debug;
#endif
};
}  // namespace BR