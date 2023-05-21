# BR-VK renderer

# Overview of Vulkan Objects

## BR::AppState
- Singleton class containing:
    - vk::Instance
    - vk::PhysicalDevice/vk::Device
    - vk::SurfaceKHR
    - vk::SwapchainKHR
    - BR::DescMgr
    - BR::SyncMgr
    - BR::MemoryMgr

- This is a singleton because architecturally the system initializes all Vulkan state in one place, and components of the system access the state as needed. Ensures the vulkan state may only be initialized once. Can have multiples of each VkObject as needed, for multi-gpu rendering, multi-surface rendering, etc

## vk::Instance
- Initialization of the Vulkan library
- Application "introduces itself" to the driver with information including engine, application name, version, which allows the driver to apply vendor optimizations
- Initialize **instance extensions** and layers
    - extensions extend vulkan functionality, require hardware support
    - layers insert themselves between the application and driver, and provide functionality such as validation and debugging
- Enable validation layers for debug build
- Enable **instance extensions** required by GLFW and Debug utilities

## vk::Device
- Vulkan has **logical** and **physical** devices 
- The physical device is the actual hardware i.e. RTX 3080
- The logical device is an instance of this hardware - an application could potentially be using multiple instances of the same hardware
- Here we have to request the queue families, and queues that we plan to use ( graphics queue )
- We request device features ( vulkan 1.3, Ray tracing pipeline, acceleration structures )
- We enable **device extensions** ( Swapchain, acceleration structure, ray tracing pipeline, buffer device address, deferred host operations, descriptor indexing, SPIRV 1.4, shader float controls )

## Queue Families for the RTX 3080:
- Queue number: 16 -> flags: Graphics Compute Transfer SparseBinding
- Queue number: 2 -> flags: Transfer SparseBinding
- Queue number: 8 -> flags: Compute Transfer SparseBinding

- 3 types of families, each supporting those usages
- Can have up to Queue Number of queues for each type
- In other words, I can have 16 graphics queues
- Queue executes command buffers (could be more than one)
- Work can be submitted to a queue from one thread at a time
- Different threads can submit work to different queues in parallel
- Multiple Queues may correspond to distinct hardware queues, or may all go to the same hardware-side queue. The hardware-level queue parallelism is not exposed to Vulkan, this is hardware dependant
 
## vk::Surface

- The connection between Vulkan and the OS window system.
- We simply use GLFW to create the window surface

## vk::Swapchain

- We request the swapchain from the graphics driver. The application does not own this memory. The driver talks to the Win32 API to facilitate displaying the image on screen
- This is probably in VRAM, but it's implementation dependant

### Vulkan Image memory model
- vk::Memory is a sequence of N bytes
- vk::Image adds information about the format, which allows access by texel, RGBA block, etc - what does this memory represent?
- vk::ImageView - similar to stringView or arrayView, it's a view into image memory, selects a part of the underlying vk::Image
- vk::RenderPass - memory map for the operation we're performing
- vk::Framebuffer - connection between the RenderPass and the memory - in this case, the Swapchain image views

## vk::RenderPass
- This defines the memory operations that will occur during the execution
- An **attachment** is a description of a resource used during rendering. I guess this is like a bind point - we're telling vulkan we're planning to do things to the color attachment, and we later **bind** an ImageView (actual memory) to that attachment. Attachments are implementation defined, there's color,depth,stencil, etc. 
- They can be directly referenced in shaders 
- i.e. layout(location = 0) out vec4 outColor
- We tell Vulkan what attachments we're planning to use, the fact that we're doing rendering in a single sub-pass, and dependencies in memory flow

## vk::Framebuffer
- Connects the RenderPass with actual memory - in this case, the Swapchain ImageViews
- Points the attachment to memory locations


# Rasterisation Pipeline

## 1. Draw
- Start of the pipeline
- Small processor inside the device interprets the commands in the command buffer and interacts with the hardware to do the work

## 2. Input assembly
- Read index and vertex buffers
- How that's done is specified by topology
- Primitive restart allows multiple strips/fans to be drawn in a single draw call, i.e. GPU knows when a new strip/fan begins
1. List topology 
- points ( single vertices )
- lines ( pairs of vertices)
- triangles ( triplets of vertices )
2. Strips/Fans 
- line strip
- triangle strip
- triangle fan
3. Adjacency primitives
- Give additional info to geometry shader, about adjacent primitives
4. Topology patch list
- Used when we use tessellation shader

## 3. Vertex shader
- Perform transformations to the vertex data

## 4. Tessellation Primitive Generation
- Fixed function, breaks patch primitives into smaller primitives

## 5. Tessellation Evaluation Shader
- Runs on each generated vertex
- Similar to a vertex shader, except the data is generated instead of being read from memory
- Looks like this runs recursively, splitting geometry further and further, on a fixed function unit

## 6. Geometry Shader
- Operates on full primitives ( points, lines, triangles )
- Can change primitive type mid-pipeline, i.e. convert triangles to lines
- Can create and destroy new geometry


## 7. Primitive Assembly
- Geometry processing is done
- Group the vertices produced by the above into primitives
- Clips and culls primitives, transforms into viewport
- **Primitive refers to a sequence of vertices**

### Clipping and Culling
- Fixed function, determines which primitives might contribute to the image, discards everything else
- Forwards the visible primitives to the rasterizer

### Algorithm
- Test each vertex against each viewport plane ( 6 sides )
- Inside, keep
- Outside, cull
- Both, clip ( cut along the plane )
- Transform to NDC ( We're still in 3D ), this is between -1 and 1
- Transform to framebuffer coordinates ( relative to the origin of the framebuffer )


## 8. Rasterizer
- Takes primitives and turns them into fragments
- Projects triangles onto the screen, using perspective projection - 3D to 2D
- Figure out which pixels the triangle covers, color the pixels
- Iterate over every triangle
- **Simply determine which pixels are inside each triangle**



### More Rasterization notes
- Algo hasn't really changed since the 1980s
- Solves the visibility problem - which parts of 3D objects are visible to the camera
- Can solve 2 ways
1. Shoot ray through every pixel, find the closest intersection ( ray tracing alg )
    - This is **image-centric** approach because we shoot rays from the camera into the scene

```
for each pixel
    for each triangle
        run intersection alg
```
2. Do the opposite **object-centric** 
    - Project the triangles onto the screen - 3D to 2D
    - Fill in the pixels of the image that are covered by that triangle

```
for each triangle
    for each pixel
        is the pixel inside the triangle?
```

- We can optimize this trivial algorithm by putting bounding boxes around the triangles
- Minimum/maximum coordinates in the triangle, use 4 coordinates
 

### Raster modes
#### depthClampEnable
- fragments that would have been clipped by near/far planes instead get projected onto those planes
- used for polygon capping

#### Culling
- throw away front or back faces
- depends on the winding order of the vertices

#### Depth Biasing
- Allows fragments to be offset in depth before running the test, which fixes z-fighting


###  Fragment
- The set of data necessary to generate a single pixel
1. Raster Position
2. Depth
3. Color/texture/etc
4. Stencil
5. Alpha


## 9. Pre Fragment Operations
- Perform depth and stencil tests once fragment positions are known
- Most implementations prefer to run these before the fragment shader, avoids unnecessary shader execution on failed fragments

## 10. Fragment Assembly
- Take the output of the rasterizer and send it to the fragment shading stage

## 11. Fragment Shader
- Perform computations on each fragment

## 12. Post Fragment Operations
- Sometimes the fragment shader might modify stencil or depth data
- In that case, we have to run those operations after the fragment shader

## 13. Color Blend
- Apply color operations to the final result + post fragment operation output and use to update the framebuffer
- Perform blending and logic operations
- Allows mixing output pixels with pixels already in the framebuffer, instead of overwriting







# Raytracing Pipeline
- Geometry goes into an acceleration structure
- This is essentially a BVH ( Bounding Volume Hierarchy )
- Use fixed-function hardware to trace rays against the BVH
- This hardware is very good at computing triangle-ray intersections

