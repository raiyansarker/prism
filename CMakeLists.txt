cmake_minimum_required(VERSION 3.10)

project(PacMan C)

# Use C11 standard
set(CMAKE_C_STANDARD 11)

# Where to put the output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Source files for the game
set(SOURCES
    src/app.c
    src/platform.c
)

# Create the library
add_library(game_lib STATIC ${SOURCES})
target_include_directories(game_lib PUBLIC src)

# Show warnings
if(MSVC)
    target_compile_options(game_lib PRIVATE /W4)
else()
    target_compile_options(game_lib PRIVATE -Wall -Wextra)
endif()

# Create the game executable
add_executable(pacman src/main.c)
target_link_libraries(pacman PRIVATE game_lib)

# Copy sounds folder to where the game runs
add_custom_command(TARGET pacman POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sounds"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sounds"
)

# Link libraries needed by each platform
if(WIN32)
    target_link_libraries(game_lib PRIVATE winmm)
else()
    target_link_libraries(game_lib PRIVATE m)
endif()
