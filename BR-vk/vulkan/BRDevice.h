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
    ~Device();

    void create( const std::vector<const char*>& deviceExtensions );
    void destroy();
    VkPhysicalDevice getPhysicaDevice();
    VkDevice getLogicalDevice();
    VkQueue getGraphicsQueue();
    int getFamilyIndex();

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_logicalDevice;
    VkQueue m_graphicsQueue;
    int m_index;
};
}  // namespace BR
