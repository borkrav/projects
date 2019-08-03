An implementation of "Raytracing in one weekend/Raytracing: The next week " in Vulkan RTX

* Based off Nvidia's Vulkan RTX tutorial (https://developer.nvidia.com/rtx/raytracing/vkray)
* I plan to continuously develop this, adding more and more rendering techniques

Overall goals:
* full pathtracer with all basic effects from books
* accumulation buffer to keep interactive framerates
* support for spheres, constructive solid geometry, cylinder, cone, other procedurals
* slower effects such as spectral rendering and sub-surface scattering
* BRDF material support

I chose Vulkan because I'm already familiar with OpenGL, and I'd like to learn a modern API  
I chose RTX because I want to use the fastest hardware-accelerated technology currently available, rather than do an AVX or CUDA implementation  


Features so far:
* Sphere intersection (AABB BLAS/TLAS support)
* Screenshot functionality (framebuffer output)
* Accumulation buffer
* Diffuse materials