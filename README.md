# Rendering Random Things

Simple realtime CPU Raytracer.

Experimenting with modern C++, various graphics libraries and nonrealistic rendering ideas. Started off with C++20 modules, but dropped them along the way due to poor Intellisense support.

Not a path-tracer and not trying to solve the rendering equation (for now), so no stochastic methods, temporal blurring or denoising utilized. Currently it's a simple realtime raytracer with reflections and fake soft shadows. Has basic features like mesh loading and SAH BVH etc.

Soft shadows are implemented by dilating shadows inwards based on a view space lighting BVH.\
Looks nice enough for a realtime CPU trick.
<br />
<br />

https://github.com/yuugencode/renderingrandomthings/assets/146561683/19a39984-7631-4ca9-97c2-c26dc8914f7b

<br />

1080p screenshot.

![rtt](https://github.com/yuugencode/renderingrandomthings/assets/146561683/4a8149e7-f9ca-49b3-8818-1d653bee014b)

<br />


Some files of interest:\
\
[Shaders.cpp](src/Rendering/Shaders.cpp) (CPU Implemented "shaders")\
[main.cpp](main.cpp) (Main loop and setup)\
[Bvh.cpp](src/Engine/Bvh.cpp) (Triangle bvh)\
[Raytracer.cpp](src/Rendering/Raytracer.cpp) (Raytracer logic)

Exists mostly for portfolio purposes, so no build files or anything included.

Uses [bgfx](https://github.com/bkaradzic/bgfx), [SDL2](https://github.com/libsdl-org/SDL), [glm](https://github.com/g-truc/glm), [ufbx](https://github.com/ufbx/ufbx), [mimalloc](https://github.com/microsoft/mimalloc), [stb_image and resize](https://github.com/nothings/stb).
