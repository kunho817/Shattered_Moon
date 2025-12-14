cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Project Definition
# ============================================================================
project(ShatteredMoon
    VERSION 0.1.0
    DESCRIPTION "Shattered Moon - DirectX 12 Game Engine"
    LANGUAGES CXX
)

# ============================================================================
# C++ Standard Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Output Directories
# ============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Windows/DirectX Settings
# ============================================================================
if(WIN32)
    # Windows SDK path (DirectX 12 is included in Windows SDK)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )

    # Enable Unicode
    add_compile_options(/utf-8)
endif()

# ============================================================================
# External Dependencies (FetchContent)
# ============================================================================
include(FetchContent)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.6
)
FetchContent_MakeAvailable(imgui)

# Create ImGui library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    d3d12
    dxgi
)

# ============================================================================
# Engine Core Library
# ============================================================================
add_library(ShatteredMoonCore STATIC)

target_sources(ShatteredMoonCore PRIVATE
    # Core
    src/core/Engine.cpp
    src/core/Window.cpp
    src/core/Memory.cpp
    src/core/FileSystem.cpp
    src/core/AssetLoader.cpp
    src/core/ResourceManager.cpp

    # ECS
    src/ecs/World.cpp
)

target_include_directories(ShatteredMoonCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ShatteredMoonCore PUBLIC
    # DirectX 12 Libraries
    d3d12
    dxgi
    d3dcompiler
    dxguid
    # ImGui
    imgui
)

# ============================================================================
# Main Executable
# ============================================================================
add_executable(ShatteredMoon WIN32
    src/main.cpp
)

target_link_libraries(ShatteredMoon PRIVATE
    ShatteredMoonCore
)

# ============================================================================
# Subdirectories (for future expansion)
# ============================================================================
# add_subdirectory(src/ecs)
# add_subdirectory(src/renderer)
# add_subdirectory(src/pcg)
# add_subdirectory(src/gameplay)
# add_subdirectory(src/editor)
# add_subdirectory(tests)

# ============================================================================
# Copy Assets and Shaders to Output Directory
# ============================================================================
add_custom_command(TARGET ShatteredMoon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ShatteredMoon>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:ShatteredMoon>/shaders
    COMMENT "Copying assets and shaders to output directory"
)

# ============================================================================
# IDE Organization
# ============================================================================
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(imgui PROPERTIES FOLDER "External")
set_target_properties(ShatteredMoonCore PROPERTIES FOLDER "Engine")
set_target_properties(ShatteredMoon PROPERTIES FOLDER "Engine")

# ============================================================================
# Installation (optional)
# ============================================================================
install(TARGETS ShatteredMoon
    RUNTIME DESTINATION bin
)

install(DIRECTORY assets/ DESTINATION bin/assets)
install(DIRECTORY shaders/ DESTINATION bin/shaders)

message(STATUS "==============================================")
message(STATUS "Shattered Moon Engine Configuration")
message(STATUS "==============================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==============================================")
