# Rendering Random Things

Simple real-time CPU Ray Tracer.

Experimenting with modern C++, various graphics libraries and nonrealistic rendering ideas. Started off with C++20 modules but dropped midway due to poor Intellisense support.

Not a path-tracer and not trying to solve the rendering equation (for now), so no stochastic methods, temporal blurring or denoising utilized. Currently it's a simple raytracer with reflections and fake soft shadows. Has basic features like mesh loading and SAH BVH etc.

Soft shadows are implemented via dilating shadows inwards based on a precomputed view space shadow point BVH.\
Works pretty well for a realtime trick running only on CPU.

https://github.com/yuugencode/renderingrandomthings/assets/146561683/ab5490fb-8a82-419e-8bd0-c3038faa589e

Some files of interest:\
\
[Shaders.cpp](src/Rendering/Shaders.cpp) (CPU Implemented "shaders")\
[main.cpp](main.cpp) (Main loop and setup)\
[Bvh.cpp](src/Engine/Bvh.cpp) (Triangle bvh)\
[Raytracer.cpp](src/Rendering/Raytracer.cpp) (Raytracer logic)

Exists mostly for portfolio purposes, so no build files or anything included.

Uses bgfx, SDL2, glm, ufbx, mimalloc, stb_image and resize.
