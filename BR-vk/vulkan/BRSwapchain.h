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

    void create( GLFWwindow* window, Surface& surface );
    void createImageViews();
    void destroy();

    VkSwapchainKHR get();
    VkFormat getFormat();
    VkExtent2D& getExtent();
    std::vector<VkImageView>& getImageViews();

    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
};
}  // namespace BR
