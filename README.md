# Voxel World

## 🖱️Mouse and ⌨️Keyboard Controls
### 🎮 Basic Controls
- **W / A / S / D** – Move the player (forward, left, backward, right)
- **Spacebar** – Jump

### 🖱 Mouse Actions
- **Left Click** – Break (destroy) the targeted block
- **Right Click** – Place a block at the targeted location

### 🔧 Toggle Features
- **[1]** – Toggle **wireframe mode** (visualize mesh edges)
- **[2]** – Toggle **physics simulation** on/off
- **[3]** – Toggle **block highlight** for hovered block
- **[4]** – Toggle **background music**

## Feature Overview
- texture atlansing
- face culling
- spawning non-block geometries, such as grass
- procedural terrain generation with simple noise
- basic physics: falling and accelerating
- AABB collision check
- DDA raycasting
- block placing and breaking
- Implemetation of OpenAL for Sound Effects and Musics

## How to Build a Project
>[!NOTE]
> Requirement:
>
> vcpkg, CMake, Git
There are two options to use vcpkg:
1. If you have Visual Studio installed, most likely the **VCPKG_ROOT** environment variable will already exist in **Developer Command Prompt for VS**
2. If you want use **vcpkg**, install **vcpkg** from git to you system:
```PowerShell
cd C:/{your path to install vcpkg}
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

After installing **vcpkg**, setup env variable **VCPKG_ROOT** and add it to **PATH**:
```PowerShell
$env:VCPKG_ROOT = "C:\{your path to}\vcpkg"
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"
```
>[!TIP]
>For troubleshooting you can read full [documentation](https://learn.microsoft.com/ru-ru/vcpkg/get_started/get-started?pivots=shell-powershell) for **vcpkg**

After installing **vcpkg** you can build project:
```PowerShell
git clone --recursive https://github.com/sarahyoo011725/VoxelWorld.git
cd VoxelCore
cmake --preset default-vs-msvc-windows
cmake --build --preset default-vs-msvc-windows
```

## Dev TO-DOs
- custom perlin noise
- fix mesh generation across chunk
- optimize storing block data: instead of storing all block types in an array, store them in a range. ex) a block in [1 ~ n] range is grass.
- optimize storing music files (musics take a lot of space)
- make raycasting align with crosshair
- more precise collision check between the player and blocks: swept AABB vs AABB
- sort objects with transparency based on their distance to the player
- efficient structure generation
- add more non-block structures like flowers
- water and leaves waving effect
- add GUI; such as inventory system.
- add lighting
- add player avatar
- biome based on temperature
