# Voxel World
Click the image to watch the video demo.

[![Voxel World Video Demo](https://github.com/user-attachments/assets/c04e0d06-ce54-4822-be0e-133506768bb1)](https://www.youtube.com/watch?v=Jtg4xtUPc4M)

## 🖱️Mouse and ⌨️Keyboard Controls
### 🎮 Basic Controls
- **W / A / S / D** – Move the player (forward, left, backward, right)
- **Spacebar** – Jump

### 🖱 Mouse Actions
- **Left Click** – Break (destroy) the targeted block
- **Right Click** – Place a block at the targeted location

### 🔧 Toggle Features
- **Control** - Toggle **speed up** on/off
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

## How to Build and Run

Windows only for now. The game targets OpenGL 3.3 core.

### 1. Prerequisites

| Tool | Notes |
| --- | --- |
| [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with the **Desktop development with C++** workload | The Build Tools edition is enough if you only plan to use VS Code. This provides the MSVC compiler and the Windows SDK. |
| [CMake](https://cmake.org/download/) 3.21+ | Bundled with the C++ workload above. |
| [Git](https://git-scm.com/downloads) and [Git LFS](https://git-lfs.com/) | **Git LFS is required** — the music and sound effects (~235 MB) are stored in LFS. |
| [vcpkg](https://github.com/microsoft/vcpkg) | Fetches glad, GLFW, GLM, OpenAL Soft and libsndfile. |

### 2. Set up vcpkg

Skip this if `VCPKG_ROOT` is already set (Visual Studio installs a copy, and its
**Developer Command Prompt** usually sets the variable for you).

```PowerShell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\dev\vcpkg", "User")
```

Open a new terminal afterwards so the variable takes effect.

> [!TIP]
> Full vcpkg setup documentation is [here](https://learn.microsoft.com/vcpkg/get_started/get-started?pivots=shell-powershell).

### 3. Clone

```PowerShell
git lfs install
git clone https://github.com/sarahyoo011725/VoxelWorld.git
cd VoxelWorld
```

If you cloned before installing Git LFS, run `git lfs pull` to replace the audio
placeholder files. The game prints a warning at startup if you forget.

### 4. Build and run

**From the command line:**

```PowerShell
cmake --preset default-vs-msvc-windows
cmake --build --preset default-vs-msvc-windows
.\build\Release\VoxelWorld.exe
```

The first configure takes a while — vcpkg builds all dependencies from source.

**From VS Code:**

1. Open the folder in VS Code and install the recommended extensions when prompted
   (C/C++ and CMake Tools).
2. When CMake Tools asks for a configure preset, pick **Visual Studio 2022 (x64)**.
3. Press <kbd>F5</kbd> to build and play.

**From Visual Studio:** open `VoxelGame.sln` and press <kbd>F5</kbd>, or open the
folder directly to use the CMake presets.

### Troubleshooting

| Symptom | Fix |
| --- | --- |
| `Could not find vcpkg` during configure | `VCPKG_ROOT` is unset or points at the wrong folder. See step 2, and open a fresh terminal. |
| `Could not find the Resources folder` | Run the executable from the repository root, or copy `Resources/` next to it. |
| Crash on startup with no message, or the LFS warning | Audio files were not fetched: `git lfs install && git lfs pull`. |
| `Failed to create a window` | Your GPU/driver does not expose OpenGL 3.3 core. Update your graphics drivers. |

## Dev TO-DOs
- custom perlin noise
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
