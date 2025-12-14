# ============================================================================
# gendev - Shattered Moon Development Assistant Tool
# ============================================================================
cmake_minimum_required(VERSION 3.20)

project(gendev
    VERSION 0.1.0
    DESCRIPTION "Shattered Moon Engine Development Tool"
    LANGUAGES CXX
)

# ============================================================================
# C++ Standard Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Platform-specific Settings
# ============================================================================
if(WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
    add_compile_options(/utf-8)
endif()

# ============================================================================
# Source Files
# ============================================================================
set(GENDEV_SOURCES
    src/main.cpp
    src/CLI.cpp
    src/FileSearch.cpp
    src/AskCommand.cpp
    src/CodeGenerator.cpp
    src/CreateCommand.cpp
)

set(GENDEV_HEADERS
    src/CLI.h
    src/FileSearch.h
    src/AskCommand.h
    src/CodeGenerator.h
    src/CreateCommand.h
)

# ============================================================================
# Executable
# ============================================================================
add_executable(gendev ${GENDEV_SOURCES} ${GENDEV_HEADERS})

# ============================================================================
# Include Directories
# ============================================================================
target_include_directories(gendev PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Compiler Features
# ============================================================================
target_compile_features(gendev PRIVATE cxx_std_20)

# ============================================================================
# Platform-specific Libraries
# ============================================================================
if(WIN32)
    # Windows console application
    set_target_properties(gendev PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# ============================================================================
# Output Directory
# ============================================================================
set_target_properties(gendev PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Copy Templates to Output Directory
# ============================================================================
add_custom_command(TARGET gendev POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/templates
        $<TARGET_FILE_DIR:gendev>/templates
    COMMENT "Copying gendev templates to output directory"
)

# ============================================================================
# Installation
# ============================================================================
install(TARGETS gendev
    RUNTIME DESTINATION bin
)

install(DIRECTORY templates/
    DESTINATION bin/templates
)

# ============================================================================
# IDE Organization
# ============================================================================
source_group("Source Files" FILES ${GENDEV_SOURCES})
source_group("Header Files" FILES ${GENDEV_HEADERS})

# ============================================================================
# Status Message
# ============================================================================
message(STATUS "==============================================")
message(STATUS "gendev Configuration")
message(STATUS "==============================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "==============================================")
