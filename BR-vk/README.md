# Vulkan Renderer

# Goals
- OBJ viewer, supporting Raster and RT
- Learn the computer graphics pipeline and GPU
- Learn rendering methods and graphics math
- Write all the Vulkan code myself, to maximize learning the API. Use minimal libraries. 


# Work log
1.  [04-2022] Went through https://vulkan-tutorial.com/
2.  [07-2022] Split tutorial code into classes
3.  [08-2022] Added debugging - naming vulkan objects
4.  [08-2022] Wrote a simple GPU memory manager, read .OBJ, draw vertices
5.  [08-2022] Implemented simple transformations  ( rotate object ). This involved learning about descriptors
6.  [08-2022] Implemented simple shading
7.  [08-2022] Added vulkan images/storage
8.  [08-2022] Added screenshot button, this writes out back buffer to disk
9.  [08-2022] Added Dear ImGui, from the Nvidia RT tutorial
10. [09-2022] Added ray tracing pipeline (BLAS, TLAS, SBT)
11. [09-2022] Added kb+m transformations - camera and model manipulators
12. [10-2022] Added depth buffering
13. [10-2022] Accumulation buffer, glossy reflections, ambient occlusion


# Current work
1. Refactor the code, split everything into logical components. Need to have a solid foundation
    Need to re-write several classes as builders - pipeline, framebuffer, renderpass, etc
2. Add support for rendering more geometry, build up scenes, multiple index buffers, VBOs, TLAS/BLAS, etc
3. Add lights, textures, different materials
4. Experiment with deferred rendering, and more advanced RT
5. At some point, I'll run out of memory handles - will have to write a memory allocator
6. Check out VK_NV_memory_decompression/VK_NV_copy_memory_indirect (Similar to the new MS direct storage, since I have NVMe SSD)

