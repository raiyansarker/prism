cmake_minimum_required(VERSION 3.20)

project(PrismTui LANGUAGES C)

# Export compile_commands.json for IDEs and static analyzers (clangd, VS Code, etc.).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Allow CMake to look for custom find modules in our repo.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# Default to a modern C dialect without GNU extensions for consistent builds.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Place binaries inside the build tree for ease of cleanup.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Collect sources explicitly so beginners can see where to add files.
set(PRISM_TUI_SOURCES
    src/app.c
    src/platform.c
)

add_library(prism_tui STATIC ${PRISM_TUI_SOURCES})
target_include_directories(prism_tui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# Tighten warnings early while keeping the list short and readable.
if(MSVC)
    target_compile_options(prism_tui PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(prism_tui PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(prism_app src/main.c)
target_link_libraries(prism_app PRIVATE prism_tui)

# Copy sounds directory to build output
add_custom_command(TARGET prism_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sounds"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sounds"
    COMMENT "Copying sound files to build directory"
)

# Link platform-specific libraries
if(WIN32)
    # Windows: winmm for PlaySound()
    target_link_libraries(prism_tui PRIVATE winmm)
else()
    # Unix/Linux/macOS: math library for sin(), pow(), etc.
    target_link_libraries(prism_tui PRIVATE m)
endif()

# Template demonstrating how third-party libraries can be wired in later.
# Uncomment and adjust these lines once you have an actual dependency:
# find_package(ExternalLib CONFIG REQUIRED)
# target_link_libraries(prism_tui PRIVATE ExternalLib::ExternalLib)

# Alternatively, drop vendored CMake projects under vendor/<lib> and use:
# add_subdirectory(vendor/some_lib)
# target_link_libraries(prism_tui PRIVATE some_lib)

# FetchContent example (see README for details):
# include(FetchContent)
# FetchContent_Declare(
#   another_lib
#   GIT_REPOSITORY https://github.com/example/another_lib.git
#   GIT_TAG        v1.0.0
# )
# FetchContent_MakeAvailable(another_lib)
# target_link_libraries(prism_tui PRIVATE another_lib)
