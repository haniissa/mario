cmake_minimum_required(VERSION 3.20)
project(smb3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------
# Dependencies
# 
# Everything comes from MSYS2's mingw64 packages, discovered though
# plg-config. We deliberately do NOT pull in <prefix>_LDFLAGS_OTHER because 
# SDL2's .pc file puts -mwindows in there, and we want to keep the console 
# attached so printf()/stderr still show up while developing.
# -----------------------------------------------------------

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(IMATH REQUIRED Imath)
pkg_check_modules(FMT REQUIRED fmt)
pkg_check_modules(VULKAN REQUIRED vulkan)

message(STATUS "SDL2        ${SDL_VERSION}")
message(STATUS "SDL2_mixer  ${SDL2_MIXER_VERSION}")
message(STATUS "Imath       ${IMATH_VERSION}")
message(STATUS "fmt         ${FMT_VERSION}")
message(STATUS "Vulkan      ${VULKAN_VERSION}")



# ---------------------------------------------------------------------------
# vk-bootstrap (vendored, MIT)
#
# Built as its own target so the project's -Wall -Wextra -Wpedantic never apply
# to third-party code -- vendored sources always warn about something, and
# silencing them by weakening our own flags would be the wrong trade.
# ---------------------------------------------------------------------------
add_library(vk_bootstrap STATIC external/vk-bootstrap/VkBootstrap.cpp)
target_include_directories(vk_bootstrap PUBLIC external/vk-bootstrap ${VULKAN_INCLUDE_DIRS})

# ---------------------------------------------------------------------------
# Shaders -> SPIR-V
#
# Compiled into the runtime output directory so they land next to mario.exe and
# resolveAsset() finds them without any path juggling.
# ---------------------------------------------------------------------------
find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslang)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found (pacman -S mingw-w64-x86_64-glslang)")
endif()

set(SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
set(SHADER_OUTPUTS 
    ${SHADER_DIR}/frame.vert.spv
    ${SHADER_DIR}/frame.frag.spv)

foreach(shader frame.vert frame.frag)
    add_custom_command(
        OUTPUT ${SHADER_DIR}/${shader}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DIR}
        COMMAND ${GLSLANG_VALIDATOR}  -V ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}
            -o ${SHADER_DIR}/${shader}.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}
        COMMENT "Compiling shader/${shader} -> SPIR-V"
        VERBATIM
    )
endforeach()

#-------------------------------------------
# Sources 
#-------------------------------------------
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Texture.cpp
    src/core/Input.cpp
    src/core/Camera.cpp
    src/core/Audio.cpp
    src/gfx/Tileset.cpp
    src/gfx/SpriteAtlas.cpp
    src/gfx/Animation.cpp
    src/gfx/BitmapFont.cpp
    src/gfx/VulkanPresenter.cpp
    src/world/TileMap.cpp
    src/world/Level.cpp
    src/world/Collision.cpp
    src/entity/Player.cpp
    src/entity/Enemies.cpp
    src/entity/Pickups.cpp
    src/entity/Effects.cpp
    src/game/World.cpp
    src/game/Hud.cpp
)
add_executable(mario ${SOURCES} ${SHADER_OUTPUTS})
target_include_directories(mario PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${IMATH_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS}
    ${VULKAN_INCLUDE_DIRS}
)
target_link_directories(mario PRIVATE 
    ${SDL2_LIBRARY_DIRS}
    ${SDL2_MIXER_LIBRARY_DIRS}
    ${IMATH_LIBRARY_DIRS}
    ${FMT_LIBRARY_DIRS}
    ${VULKAN_LIBRARY_DIRS}
)

target_link_libraries(mario PRIVATE 
    ${SDL2_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${IMATH_LIBRARIES}
    ${FMT_LIBRARIES}
    ${VULKAN_LIBRARIES}
    vk_bootstrap
)

# //-Dmain=SDL_main and firends, but not -mwindows (see the note above: SDL2_mixer's)
# .pc file carries the same flag, so it is excluded here too
target_compile_options(mario PRIVATE 
    ${SDL2_CFLAGS_OTHER} ${SDL2_MIXER_CFLAGS_OTHER} ${IMATH_CFLAGS_OTHER}
    ${FMT_CFLAGS_OTHER}
)

if(MSVC)
    target_compile_options(mario PRIVATE /W4)
else()
    target_compile_options(mario PRIVATE -Wll -Wextra -Wpedantic)
endif()

# -------------------------------------------------------------
# Copy the runtime DLLs next to the executable so it can be lanuched directly
# from Explorer, not just from an MSYS2 shell.
# -------------------------------------------------------------
if(WIN32)
    add_custom_command(TARGET mario POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DEXE=$<TARGET_FILE:mario>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntimeDeps.cmake
                COMMENT "Regenerating sprite atls from ASCII art"
                VERBATIM
    )
endif()
# function(steup_lib lib_name repo_path git_tag_version)
# endfunction()
