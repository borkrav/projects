#pragma once

#include <BRDebug.h>
#include <BRDescMgr.h>
#include <BRDevice.h>
#include <BRInstance.h>
#include <BRMemoryMgr.h>
#include <BRSurface.h>
#include <BRSwapchain.h>
#include <BRSyncMgr.h>

#include <vulkan/vulkan_handles.hpp>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace BR
{
/*

Singleton class defining the Vulkan application state, containing:

1 VkInstance
1 VkPhysicalDevice/VkDevice
1 VkSurfaceKHR
1 VkSwapchainKHR

// This can be extended to support multiple devices, multiple Surfaces + Swapchains
// For now, this is not supported, only supporting 1 GPU rendering to 1 window

Class also contains

DescMgr - Manages Description sets/layouts
SyncMgr - Manages Sync objects
MemoryMgr - Manages GPU memory

Each of these classes keep track of created objects, and destroy them
Only need one of each for this project

*/

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
    vk::Image getSwapchainImage( int index );
    vk::Format getSwapchainFormat();
    vk::Extent2D& getSwapchainExtent();
    std::vector<vk::ImageView>& getImageViews();
    BR::DescMgr& getDescMgr();
    BR::SyncMgr& getSyncMgr();
    BR::MemoryMgr& getMemoryMgr();
    GLFWwindow* getWindow();

    void takeScreenshot( CommandPool& pool, int frame );

    uint32_t m_iWidth = 1920;
    const uint32_t m_iHeight = 1080;
    const int m_framesInFlight = 2;

    //RT Device Functions
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR
        vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR
        vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR
        vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

    //RT Device Properties
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR
        rayTracingPipelineProperties;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR
        accelerationStructureFeatures;

#ifdef NDEBUG
#else
    BR::Debug getDebug();
#endif

   private:
    AppState();
    ~AppState();

    void initRT();
    void getFunctionPointers();

    GLFWwindow* m_window;

    //TODO: need to clean these classes up, move things to constructors
    BR::Instance m_instance;
    BR::Device m_device;
    BR::Surface m_surface;
    BR::Swapchain m_swapchain;
    BR::DescMgr m_descMgr;
    BR::SyncMgr m_syncMgr;
    BR::MemoryMgr m_memoryMgr;

#ifdef NDEBUG
#else
    BR::Debug m_debug;
#endif
};
}  // namespace BR