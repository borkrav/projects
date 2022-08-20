Vulkan Renderer
- Work with OBJ files and scenes
- Draw triangle scene
- Learn lighting/materials/math/shaders
- Ray Tracing mode, using RTX hardware, Vulkan extensions
- Ray Tracing will implement bounce lighting, shadows, etc


- Goal is to be very familiar in how Vulkan works, how it maps to GPU
- Learn some math
- Learn some rendering methods - lighting, shading, etc


1. Split current tutorial code into distinct classes for readability/understandability. Going to get it to work on my own hardware, not going to care about the general case - DONE
2. Write a debug annotation utility - DONE
    - write a simple GPU memory manager - read OBJ, draw the vertices  - DONE
    - refactor the memory manager - DONE
    - figure out transformations of your object - DONE
    - implement simple flat shading

    - write an image creator
    - write a pipeline generator

3. Add Dear ImGui, follow tutorial, understand how this works
4. Read .OBJ file, create buffers, similar to Nvidia Tutorial, draw the cube, try other scenes
5. Start learning about lighting/shading
6. Add in screenshot from other tutorial
7. Add in ray tracing
8. Add accumulation buffer, from other tutorial


