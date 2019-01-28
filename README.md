# Adypt
AdamYuan's PathTracer on GPU. Implemented with OpenGL 4.5.

## Compilation
```bash
cmake . -DCMAKE_BUILD_TYPE=Release
make
```

## Usage
```bash
./Adypt [*.config]
```
* W, A, S, D, L-Shift, Space: Move around
* Esc: Toggle window focus
* R: Toggle path tracing
* P: Save image to exr file

## Built With
* [GL3W](https://github.com/skaslev/gl3w) - For modern OpenGL methods
* [GLFW](http://www.glfw.org/) - Window creation and management
* [GLM](https://glm.g-truc.net/) - Maths calculations
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) - Image loading
* [TinyOBJLoader](https://github.com/syoyo/tinyobjloader) - Obj loading
* [TinyEXR](https://github.com/syoyo/tinyexr) - EXR file saving


## Reference
* https://research.nvidia.com/publication/2017-07_Efficient-Incoherent-Ray - Compressed Wide BVH
* https://github.com/AlanIWBFT/CWBVH/blob/master/src/TraversalKernelCWBVH.cu - Compressed Wide BVH traversal implementation
* https://code.google.com/archive/p/understanding-the-efficiency-of-ray-traversal-on-gpus/ - Spatial Split BVH building
* http://e-maxx.ru/algo/assignment_hungary#6 - Elegant implementation of hungarian assignment algorithm

## Screenshots
![](https://raw.githubusercontent.com/AdamYuan/Adypt/master/screenshots/firepalace3.png)
![](https://raw.githubusercontent.com/AdamYuan/Adypt/master/screenshots/livingroom1.png)
![](https://raw.githubusercontent.com/AdamYuan/Adypt/master/screenshots/cornell_water1.png)
![](https://raw.githubusercontent.com/AdamYuan/Adypt/master/screenshots/sponza3.png)
![](https://raw.githubusercontent.com/AdamYuan/Adypt/master/screenshots/san3.png)

