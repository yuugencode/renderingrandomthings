# Rendering Random Things

Simple realtime toy CPU Raytracer.

Experimenting with modern C++, various graphics libraries and nonrealistic rendering ideas. Started off with C++20 modules, but dropped them along the way due to Intellisense still not properly supporting them.

Not a path-tracer and not trying to solve the rendering equation (for now), so no stochastic methods, temporal blurring or denoising utilized. Currently it's a simple realtime whitted-style raytracer with reflections, fake soft shadows and fake indirect lighting. Has basic features like mesh loading and SAH BVH etc.

Soft shadows are implemented by dilating shadows inwards based on a view space lighting BVH, which looks alright for a realtime CPU trick.\
Indirect lighting is an analytical solution done by calculating potential reflection points on nearby objects. Optimizing this is @TODO.

Exists mostly for personal tinkering and portfolio purposes.

Potential files of interest:\
[Shaders.cpp](src/Rendering/Shaders.cpp) (CPU Implemented "shaders")\
[Raytracer.cpp](src/Rendering/Raytracer.cpp) (Raytracer logic)\
[Bvh.cpp](src/Engine/Bvh.cpp) (Triangle bvh)\
[main.cpp](main.cpp) (Main loop and setup)

Uses the following libraries [bgfx](https://github.com/bkaradzic/bgfx), [SDL2](https://github.com/libsdl-org/SDL), [glm](https://github.com/g-truc/glm), [ufbx](https://github.com/ufbx/ufbx), [mimalloc](https://github.com/microsoft/mimalloc), [stb_image and resize](https://github.com/nothings/stb).


https://github.com/yuugencode/renderingrandomthings/assets/146561683/909187e2-9ecf-4dd1-a625-6fa2f07fafaf

<br />

https://github.com/yuugencode/renderingrandomthings/assets/146561683/00e5f6d4-5060-43f1-8e4b-c3954abc3b07

<br />

1080p screenshot.
![rt](https://github.com/yuugencode/renderingrandomthings/assets/146561683/8627f052-f7c3-4da9-8c55-01101cc2ee78)

<br />

