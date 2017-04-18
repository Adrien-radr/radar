### General
+ ~~Refactor Water code somewhere~~
+ ~~Think about compiling .cpp's separately --> No~~
+ ~~Fix OpenAL using Jack by default when building it on Debian, wtf is this shit. Turns out it's
just trying to connect to jack before the other drivers and jack is verbose...~~
+ ~~Remove all {m,c}alloc from the code except the Pools creation~~
- Fast, Deterministic Random Variable system --> SFMT, WIP

### UI
- 2D Panel creation with custom shader
    - ~~1 VS / 1 FS : pos, tex, color~~
    - 1 VAO/VBO per panel
    - Z depth layering
    - Transparence
- Display textures in panel (e.g. gbuffers display)

### Water
- Underwater render
    - Underwater background screen quad at infinity when nothing intersected
    - Godrays
    - Caustics on ocean floor
- Screenspace solution
    - Screen-space partitionned in horiz. slabs to allow different LODs (no transparency & other
    demanding effects for far away water)
    - Screen-space quad projected onto y=0 and FFT/sim done by compute shaders ?
- Water Beaufort Scale States
    - 4/5 States, with precomputed HTidle0 (and other init data)
    - Interpolation between states at runtime
- Water / Boat / Objects collision and interactions (Gamasutra & Black Flag articles)
- Dual Depth Peeling for water transparency
