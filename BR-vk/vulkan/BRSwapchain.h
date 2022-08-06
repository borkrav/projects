#pragma once

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

    void create( GLFWwindow* window, std::string name );
    void createImageViews( std::string name );
    void destroy();

    vk::SwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::ImageView> m_swapChainImageViews;
    vk::Device m_device;
};
}  // namespace BR
