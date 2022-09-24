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

# Current work
1. Add kb+m camera transformations - orbit, zoom, pan, etc. Should teach me transforms
2. Go into Raster and RT shaders. Implement some more interesting shading
3. Add accumulation buffer from the 2019 work
4. Some refactoring and class re-design might be needed



