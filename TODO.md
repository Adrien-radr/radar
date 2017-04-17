### General
+ Refactor Water code somewhere
+ Think about compiling .cpp's separately --> No
- Fix OpenAL using Jack by default when building it on Debian, wtf is this shit.
- Fast, Deterministic Random Variable system
+ Remove all {m,c}alloc from the code except the Pools creation

### UI
- 2D Panel creation with custom shader
- Per-panel Text VAO/VBO.
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
- Water / Boat / Objects collision and interactions (Gamasutra & Black Flag articles)
