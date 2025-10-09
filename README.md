# WolfPro
A modern hardware compatible, high performance client/server and competition mod for Return to Castle Wolfenstein multiplayer. Intended to be a successor to RTCWPro. 

## Client/Server changes from Vanilla RtCW MP:

 - CMake project management
 - Docker build and dedicated server 
 - 64-bit client and server
 - Vulkan renderer backend added
	 - Dear ImGUI integration for easy UI programming and debugging
	 - Modern graphics techniques using shaders for various rendering aspects:
		 - CRT monitor-like gamma and overbrightbits behavior
		 - Dynamic light
		 - Lightmap lighting
	 - Mipmap generation moved to GPU to use better algorithms and reduce upload time
	 - Multi-Sample Anti-Aliasing (MSAA 2x-8x) support with alpha2coverage
 - Original JPEG library replaced with high performance libjpeg-turbo
 - Original Huffman encoding/decoding replaced with high performance implementation from CNQ3
 - Significant clean up to remove dead code (Singleplayer, Q3:TA, IPX, BSP Compiler)
 - Warnings and memory leaks significantly addressed/eliminated
 - Client input sampling and frame pacing optimized for low latency and smoothness
 - Console font scaling
 - Raw mouse input

## Mod changes from Vanilla:
Majority of the changes are ported from RTCW Pro. 

 - Ready/Notready
 - Pause/Unpause
 - Blood flash/damage removed
 - Custom crosshair cvars
 - New demo player with rewind support and timeline of demo recorder's perspective
 - Unlagged hitscan antilag
 - Server-side rename and client opt-out ability
 - Hitsounds
 - Built-in enemy respawn timer / share next spawntime based on your timer set
 - Weaponstats
 - End-of-round/game stats upload to web API
 - Muzzle flash for all, enemies, or none
 - Tracers for all, enemies, or none
 - Various bug fixes for artillery hitting players indoors or instantly exploding
 - Momentary +vstr bind support
 - say_teamNL (no location) support
 - Q3Shader overrides to disallow picmip for transparent surfaces
 - 

