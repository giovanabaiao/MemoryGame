# Star Wars Memory Game (C++ / SFML 3.x)

A 2D full-screen memory game built in C++ with SFML 3.x.  
The board is a single-screen 8x4 grid (32 cards total) with 16 Star Wars character pairs, flip animations, a 2-second reveal window, timer, move counter, and a New Game button.

## Features
- 2D single-screen gameplay.
- 8x4 card grid (16 pairs).
- Flip-on-click animation.
- 2-second reveal period before pair resolution.
- Move counter + elapsed timer.
- Full-screen responsive layout with proportional scaling.
- Pixel-art styled character card faces.
- Cross-platform target: macOS + Windows.

## Tech Stack
- C++17 or newer
- [SFML 3.x](https://www.sfml-dev.org/)
- CMake 3.24+
- Optional for asset tooling:
  - Python 3.10+
  - Pillow (`pip install pillow`)

## Project Layout
```text
MemoryGame/
  README.md
  DETAILED_PLAN.md
  CMakeLists.txt
  src/
  assets/
    manifest/
    source/
    processed/
    ui/
  tools/
```

## Dependencies and Setup

## macOS

### Option A: Homebrew (simple local setup)
1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. Install dependencies:
   ```bash
   brew install cmake sfml
   ```
3. Verify:
   ```bash
   cmake --version
   ```

### Option B: Build SFML 3.x from source (strict version control)
1. Install build tools:
   ```bash
   brew install cmake ninja
   ```
2. Clone SFML and build/install it (example path under `/usr/local` or your custom prefix).
3. Configure this project with `-DSFML_DIR=<path-to-sfml-cmake-config>`.

## Windows

### Option A: vcpkg (recommended)
1. Install Visual Studio 2022 with C++ workload.
2. Install [vcpkg](https://github.com/microsoft/vcpkg).
3. Install SFML:
   ```powershell
   vcpkg install sfml
   ```
4. Build project with vcpkg toolchain:
   ```powershell
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```

### Option B: Prebuilt SFML binaries
1. Download matching SFML 3.x binaries for your compiler/runtime.
2. Point CMake to SFML config:
   ```powershell
   cmake -S . -B build -DSFML_DIR=C:/path/to/SFML/lib/cmake/SFML
   cmake --build build --config Release
   ```

## Build Instructions (Both Platforms)

From project root:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run executable from the build output directory (path depends on generator and OS).

## CMake Integration Notes
- In `CMakeLists.txt`, target SFML 3:
  - `find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)`
- Link your game target with SFML imported targets.
- Keep runtime assets (`assets/`) accessible from working directory or copy them post-build.

## Asset Pipeline (Pixel Art)

This project is designed to use script-based asset preparation:
1. Download original source images for each character into `assets/source/`.
2. Convert and normalize into pixel-art-style PNG files in `assets/processed/`.
3. Load textures through a manifest (`assets/manifest/cards.json`) to keep data-driven mapping.

Planned scripts:
- `tools/fetch_assets.py` - downloads originals from a manifest URL list.
- `tools/process_assets.py` - crops/resizes/quantizes and exports final PNGs.

## Gameplay Rules (Planned)
- Click a face-down card to flip it.
- Select a second card to form a move.
- Both cards remain visible for 2 seconds.
- Matching pair disappears, non-matching pair flips back.
- Game ends when all 16 pairs are matched.

## Notes
- Character names and content are part of a Star Wars themed fan project; confirm image usage rights before distribution.
- See `DETAILED_PLAN.md` for full architecture, milestones, and implementation details.
