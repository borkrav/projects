#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "BRInstance.h"
#include "BRSurface.h"

namespace BR
{
class Device
{
   public:
    Device();
    ~Device();

    void create( Instance& instance, const std::vector<const char*>&deviceExtensions, Surface& surface);
    void destroy();
    VkPhysicalDevice getPhysicaDevice();
    VkDevice getLogicalDevice();
    VkQueue getGraphicsQueue();
    int getFamilyIndex();

   private:
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_logicalDevice;
    VkQueue m_graphicsQueue;
    int m_index;
};
}  // namespace BR
