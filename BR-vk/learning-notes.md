Vulkan function pointers
    - all non-extension functions are available from the loader (vulkan-1.dll)
    - vkGetDeviceProcAddr result only works with one device, but this is faster
    - avoids internal dispatch, call the function directly


Vulkan pointers to data ( vkGetBufferDeviceAddressKHR )
    - Device address that can be accessed in a shader
    - Binding buffers to shaders becomes as easy as sending data through a uniform
    - Avoids doing descriptor bindings
    - Need to enable this with VkPhysicalDeviceVulkan12Features::bufferDeviceAddress
    - Can pass in the address with uniform or push constants
    - Less of a headache, don't have to bind resources yourself anymore
    - Improved CPU performance, there are less API calls
    - Overhead on the GPU side because more logic involved in accessing the data, since bindings are all static
    - Required for RT, since we have no idea what material was hit, need to look it up
    - Binding textures per-draw becomes a performance hotspot - I haven't gotten to textures yet so I don't know much about this


RTX execution model
    - In rasterization, we batch objects by shaders, so we always know which shaders must be called to render the objects
    - In Ray trace, we have no idea which objects the rays will hit
    - In bindless rendering, we access textures and buffers by ID, at run time, in shader
    - We essentially now have bindless shaders, where we just call whatever is needed