# Prism TUI Scaffold

A beginner-friendly C project skeleton for building terminal user interfaces on macOS and Windows. It keeps headers next to their corresponding source files, favors a minimal directory layout, and demonstrates how to bolt on external libraries later.

## Requirements

- CMake 3.20 or newer
- A C17-capable compiler (Clang/AppleClang, MSVC, GCC)
- Optional: Ninja or another CMake generator you prefer

## Project Layout

```
prism/
├─ CMakeLists.txt        # Build configuration (macOS + Windows)
├─ cmake/Modules/        # Custom find_package helpers
│  └─ FindExternalLib.cmake (template)
├─ src/                  # App code; headers live beside sources
│  ├─ app.c / app.h
│  └─ main.c
├─ vendor/               # Drop vendored third-party projects here
└─ README.md
```

## Build & Run

### macOS / Linux

```bash
cmake -S . -B build
cmake --build build
./build/bin/prism_app
```

### Windows (PowerShell)

```powershell
cmake -S . -B build -G "Ninja"    # or "Visual Studio 17 2022"
cmake --build build
./build/bin/prism_app.exe
```

## Sample Application

`src/main.c` wires a simple loop that:

1. Greets the user and prints placeholder UI content.
2. Waits for `n` (advance the tick counter) or `q` (quit).
3. Keeps rendering until you decide to exit.

This is intentionally trivial so you have a safe place to experiment before integrating a real TUI framework.

## Adding External Libraries

You can grow the project in three common ways. Each approach is documented in comments inside `CMakeLists.txt` and the template `cmake/Modules/FindExternalLib.cmake`.

### 1. System Packages via `find_package`

1. Copy the template: `cp cmake/Modules/FindExternalLib.cmake cmake/Modules/FindMyLib.cmake`.
2. Replace the placeholder names with your library's header and library filenames.
3. In `CMakeLists.txt` add:
   ```cmake
   find_package(MyLib REQUIRED)
   target_link_libraries(prism_tui PRIVATE ${MyLib_LIBRARIES})
   target_include_directories(prism_tui PRIVATE ${MyLib_INCLUDE_DIRS})
   ```
4. Re-run the CMake configure step. If CMake can't locate the dependency, set `-DMyLib_ROOT=/path/to/install`.

### 2. Vendored Sources via `add_subdirectory`

1. Drop the dependency's CMake project into `vendor/<name>`.
2. Add `add_subdirectory(vendor/<name>)` to `CMakeLists.txt`.
3. Link it: `target_link_libraries(prism_tui PRIVATE <name>)`.

This keeps everything offline and is great for small helper libraries.

### 3. Remote Projects via `FetchContent`

1. In `CMakeLists.txt`, uncomment or adapt the provided `FetchContent` snippet:
   ```cmake
   include(FetchContent)
   FetchContent_Declare(
     cool_tui
     GIT_REPOSITORY https://github.com/example/cool_tui.git
     GIT_TAG        v1.0.0
   )
   FetchContent_MakeAvailable(cool_tui)
   target_link_libraries(prism_tui PRIVATE cool_tui)
   ```
2. Configure/build again and CMake will download + build the dependency automatically.

> Tip: keep third-party code isolated (module, vendor folder, or FetchContent block) so it is easy to swap libraries when you find the TUI stack you want.

## Next Steps

- Replace the placeholder input loop with your chosen TUI toolkit.
- Add automated tests (for example using [Unity](https://www.throwtheswitch.org/unity) or [cmocka](https://cmocka.org/)).
- Extend `FindExternalLib.cmake` into real per-library discovery scripts as needed.
