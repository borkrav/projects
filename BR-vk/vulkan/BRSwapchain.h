#pragma once

#include <BRCommandPool.h>
#include <BRDevice.h>

#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Swapchain
{
    friend class AppState;

   private:
    Swapchain();
    ~Swapchain();
    Swapchain( const Swapchain& ) = delete;

    void create( std::string name, GLFWwindow* window,
                 vk::PhysicalDevice physicalDevice, vk::Device logicalDevice,
                 int familyIndex, vk::SurfaceKHR surface );
    void createImageViews( std::string name );
    void destroy();

    void takeScreenshot( CommandPool& pool, int frame );

    vk::SwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::ImageView> m_swapChainImageViews;
    vk::Device m_device;
};
}  // namespace BR
