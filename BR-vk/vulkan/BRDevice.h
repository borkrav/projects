#pragma once

#include <BRInstance.h>
#include <BRSurface.h>

#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Device
{
    friend class AppState;

   private:
    Device();
    Device( const Device& ) = delete;

    void create( std::string name,
                 const std::vector<const char*>& deviceExtensions,
                 vk::Instance instance,
                 vk::SurfaceKHR surface);

    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_logicalDevice;
    vk::Queue m_graphicsQueue;
    int m_index;
};
}  // namespace BR
