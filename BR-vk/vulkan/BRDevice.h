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

    void create( std::string name,
                 const std::vector<const char*>& deviceExtensions );

    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_logicalDevice;
    vk::Queue m_graphicsQueue;
    int m_index;
};
}  // namespace BR
