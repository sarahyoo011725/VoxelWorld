cmake_minimum_required(VERSION 3.21)

# Pick up vcpkg automatically when VCPKG_ROOT is set. The presets already pass
# the toolchain explicitly; this covers plain `cmake -B build` invocations.
# Must happen before project().
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
			CACHE STRING "Vcpkg toolchain file")
	else()
		message(FATAL_ERROR
			"Could not find vcpkg.\n"
			"Set the VCPKG_ROOT environment variable to your vcpkg checkout, or pass\n"
			"  -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake\n"
			"See the 'How to Build and Run' section of README.md.")
	endif()
endif()

project(VoxelWorld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(VoxelWorld
	Source/Main.cpp
	Source/stb.cpp
	Source/Audio/AudioManager.cpp
	Source/Audio/MusicBuffer.cpp
	Source/Audio/SoundDevice.cpp
	Source/Audio/SoundEffectsLibrary.cpp
	Source/Audio/SoundEffectsPlayer.cpp
	Source/Buffers/EBO.cpp
	Source/Buffers/FBO.cpp
	Source/Buffers/RBO.cpp
	Source/Buffers/VAO.cpp
	Source/Buffers/VBO.cpp
	Source/Camera/Camera.cpp
	Source/Chunk/Chunk.cpp
	Source/Entity/Player.cpp
	Source/Entity/PlayerPhysics.cpp
	Source/Screens/PlayerRenderer.cpp
	Source/Shader/Shader.cpp
	Source/Texture/Texture.cpp
	Source/World/BlockInteractor.cpp
	Source/World/ChunkManager.cpp
	Source/World/StructureGenerator.cpp
	Source/World/Terrain.cpp
)

# Source/ so that includes like <Shader/Shader.h> resolve; Libraries/include for
# the vendored header-only deps (FastNoiseLite, stb_image).
target_include_directories(VoxelWorld PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Source"
	"${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include"
)

target_link_libraries(VoxelWorld PRIVATE
	glad::glad
	glfw
	glm::glm
	OpenAL::OpenAL
	SndFile::sndfile
	OpenGL::GL
)

# Last-resort fallback for locating Resources/ when the executable is run from
# somewhere that has no copy next to it (see resolve_resource_root in Main.cpp).
target_compile_definitions(VoxelWorld PRIVATE
	VOXELWORLD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
	# /utf-8: the sources are UTF-8; without it MSVC reads them as the system
	# codepage. /EHsc: the audio code throws on load failure.
	target_compile_options(VoxelWorld PRIVATE /utf-8 /EHsc)
	set_target_properties(VoxelWorld PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
endif()

# Resources/ is ~235 MB (mostly music), so it is located at runtime rather than
# copied next to the executable on every build.
