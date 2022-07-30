#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "BRDevice.h"

namespace BR
{
class Swapchain
{
   public:
    Swapchain();
    ~Swapchain();

    void create( GLFWwindow* window, Device& device, Surface& surface );
    void createImageViews(Device& device);
    void destroy( Device& device );

    VkSwapchainKHR get();
    VkFormat getFormat();
    VkExtent2D& getExtent();
    std::vector<VkImageView>& getImageViews();

   private:
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
};
}  // namespace BR
