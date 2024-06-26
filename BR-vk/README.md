# Vulkan Renderer

![RT Glossy Reflections](https://github.com/borkrav/projects/blob/master/BR-vk/screenshots/Fri-May-12-16-24-33-2023.png?raw=true)
![RT Mirror Reflections](https://github.com/borkrav/projects/blob/master/BR-vk/screenshots/Fri-May-12-16-24-27-2023.png?raw=true)
![RT Ambient Occlusion](https://github.com/borkrav/projects/blob/master/BR-vk/screenshots/Fri-May-12-16-24-18-2023.png?raw=true)
![Raster](https://github.com/borkrav/projects/blob/master/BR-vk/screenshots/Fri-May-12-16-24-10-2023.png?raw=true)

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
11. [09-2022] Added kb+m transformations - camera and model transforms
12. [10-2022] Added depth buffering
13. [10-2022] Accumulation buffer, glossy reflections, ambient occlusion
14. [12-2022] Refactored the code, broke up dependencies between raster and ray trace, re-wrote classes as builders
15. [02-2023] Added instanced drawing, for making simple scenes with multiple objects
16. [05-2023] Added simple materials - using the colors in the .mtl file

# Current work
1. Implement full support for .obj reading - multiple shapes and materials
2. Add lights, textures, different materials
3. Experiment with deferred rendering, and more advanced RT
4. At some point, I'll run out of memory handles - will have to write a memory allocator
5. Check out VK_NV_memory_decompression/VK_NV_copy_memory_indirect (Similar to the new MS direct storage, since I have NVMe SSD)