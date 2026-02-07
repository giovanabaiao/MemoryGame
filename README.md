# Star Wars Memory Game (C++ / SFML 3.x)

2D full-screen memory game built in C++ with SFML 3.x.

Current implementation includes:
- Single-screen 8x4 grid (32 cards, 16 pairs)
- Flip animation
- 2-second reveal window for each pair selection
- Matched cards disappear
- Timer and move counter
- New Game button
- Responsive full-screen layout for multiple resolutions
- Asset pipeline scripts for downloading and processing card art

## Requirements
- CMake 3.24+
- C++20 compiler
- SFML 3.x
- Python 3.10+ (for asset scripts)
- Pillow (`pip install pillow`) for `tools/process_assets.py`

## Setup

## Quick Start Script
From project root:
```bash
./run_game.sh
```

Useful options:
```bash
./run_game.sh --no-run
./run_game.sh --debug
./run_game.sh --fetch-sfml
```

## macOS
1. Install toolchain:
   ```bash
   xcode-select --install
   brew install cmake sfml python
   ```
2. Configure and build:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
3. Run:
   ```bash
   ./build/bin/memory_game
   ```

If your SFML 3 install is custom:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSFML_DIR=/path/to/SFML/lib/cmake/SFML
```

## Windows

### Option A: vcpkg (recommended)
1. Install Visual Studio 2022 (Desktop development with C++).
2. Install [vcpkg](https://github.com/microsoft/vcpkg) and SFML:
   ```powershell
   vcpkg install sfml
   ```
3. Configure and build:
   ```powershell
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```
4. Run:
   ```powershell
   .\build\bin\Release\memory_game.exe
   ```

### Option B: Prebuilt/custom SFML
```powershell
cmake -S . -B build -DSFML_DIR=C:/path/to/SFML/lib/cmake/SFML
cmake --build build --config Release
```

## Optional: Fetch SFML From CMake
If SFML is not installed globally:
```bash
cmake -S . -B build -DMEMORY_GAME_FETCH_SFML=ON
cmake --build build --config Release
```

## Asset Pipeline (Pixel Art)

## 1) Add source URLs
Edit `/Users/gigi/Programming/MemoryGame/assets/manifest/sources.json` and fill each `url`.

## 2) Download originals
```bash
python tools/fetch_assets.py
```

## 3) Process to pixel-art PNG
```bash
python tools/process_assets.py --contact-sheet
```

Generated files go to:
- `/Users/gigi/Programming/MemoryGame/assets/source`
- `/Users/gigi/Programming/MemoryGame/assets/processed`

The game auto-loads processed textures from:
- `/Users/gigi/Programming/MemoryGame/assets/processed/<slug>.png`

If textures are missing, fallback colored cards are used so the game is still playable.

## Fonts
For consistent pixel-art text, add a font file at:
- `/Users/gigi/Programming/MemoryGame/assets/fonts/PressStart2P-Regular.ttf`

The game also tries common system fonts on macOS/Windows.

## Controls
- Left click: flip card / press New Game
- Escape: quit game

## Project Files
- `/Users/gigi/Programming/MemoryGame/src/main.cpp` - current game implementation
- `/Users/gigi/Programming/MemoryGame/CMakeLists.txt` - build config
- `/Users/gigi/Programming/MemoryGame/DETAILED_PLAN.md` - long-form development plan
- `/Users/gigi/Programming/MemoryGame/tools/fetch_assets.py` - source image downloader
- `/Users/gigi/Programming/MemoryGame/tools/process_assets.py` - pixel-art converter

## Contributing
1. Create a branch:
   ```bash
   git checkout -b codex/<topic>
   ```
2. Keep changes focused.
3. Build and run locally.
4. Update docs if behavior/build steps change.
5. Open a PR with:
   - What changed
   - Why it changed
   - How you tested (platform + commands)

## Licensing and IP Note
This is a Star Wars fan project. Ensure image sources and redistribution rights are valid before publishing binaries/assets.
