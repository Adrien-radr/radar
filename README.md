## Radar Engine
A Physically based renderer in OpenGL/GLSL 4.
![Screenshot 1](https://github.com/Adrien-radr/radar/blob/master/screenshots/1.png "Dev Screenshot 1")
![Screenshot 2](https://github.com/Adrien-radr/radar/blob/master/screenshots/2.png "Dev Screenshot 2")
![Screenshot 3](https://github.com/Adrien-radr/radar/blob/master/screenshots/3.png "Dev Screenshot 3")
![Screenshot 4](https://github.com/Adrien-radr/radar/blob/master/screenshots/4.png "Dev Screenshot 4")
![Screenshot 5](https://github.com/Adrien-radr/radar/blob/master/screenshots/5.png "Dev Screenshot 5")
![Screenshot 6](https://github.com/Adrien-radr/radar/blob/master/screenshots/6.png "Dev Screenshot 6")

## Features
- OpenGL 4.x / GLSL(400) rendering engine
- Physically based rendering (GGX and Lambertian diffuse) with albedo/specular-roughness/normal maps
- Ocean rendering
- Atmospheric scattering with time of day (Sun + Moon)
- Immediate mode GUI inspired by dear-imgui
- GLTF Model suport
- Basic OpenAL support

## TODO
- Unifying ocean and atmosphere rendering for smooth water to sky transition

## Sources
This repository contains an implementation of those papers, amongst other things :
- [Simulating Ocean Water](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.161.9102&rep=rep1&type=pdf) - Jerry Tessendorf (1999)
- [Precomputed Atmospheric Scattering](https://hal.inria.fr/inria-00288758/document) - Eric Bruneton, Fabrice Neyret (2008)

## Used libraries
- [GLEW](https://github.com/nigels-com/glew)
- [GLFW 3](http://www.glfw.org/)
- [CJSON](https://github.com/DaveGamble/cJSON)
- [OpenAL](https://github.com/kcat/openal-soft)
- [STB Image & Truetype](https://github.com/nothings/stb)
- [Tiny GLTF](https://github.com/syoyo/tinygltf)
- [SFMT](http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/)
