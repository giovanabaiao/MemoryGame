# Star Wars Memory Game - Detailed Development Plan

## 1) Project Scope and Non-Negotiable Requirements

### Core Requirements
- 2D game.
- Language: C++.
- Rendering/input/audio framework: SFML 3.x.
- Platform support: macOS and Windows.
- Single screen gameplay with an 8x4 grid (32 cards total).
- 16 character pairs (2 cards per character).
- Required characters:
  - Luke Skywalker
  - Leia Organa
  - Darth Vader
  - Anakin Skywalker
  - Obi-Wan Kenobi
  - Yoda
  - Han Solo
  - Chewbacca
  - Emperor Palpatine
  - Rey
  - Kylo Ren
  - R2-D2
  - C-3PO
  - Lando Calrissian
  - Mandalorian
  - Padme Amidala
- UI must include:
  - `New Game` button
  - Timer
  - Move counter
- Card interaction:
  - Click to flip.
  - Flip animation.
  - 2-second view time after clicking.
- Full-screen responsive layout:
  - UI and board scale dynamically while preserving proportions across resolutions/aspect ratios.
- Visual style: pixel art.
- Asset pipeline requirement:
  - Download original source images.
  - Convert them to PNG with a scriptable process.

### Assumptions (to avoid ambiguity)
- A move = revealing a second card (one comparison attempt).
- Timer starts on first card interaction, not on app launch.
- 2-second view rule is implemented as: when a pair is selected, both cards stay visible for at least 2 seconds before match resolution.
- Matched cards disappear from the board after successful pair resolution.
- Unmatched cards flip back after the 2-second reveal window.

---

## 2) Target User Experience

### Gameplay Flow
1. Launch game in full-screen mode.
2. User sees title/status bar + centered 8x4 card grid.
3. User clicks one card -> card flips to front.
4. User clicks second card -> second card flips to front.
5. Both visible for 2 seconds.
6. If same character, pair disappears from the grid.
7. If different, both flip back after delay.
8. Move counter increments after second card is selected.
9. Timer runs until all 16 pairs are matched.
10. End-state message displayed with final time and moves.
11. `New Game` resets and reshuffles instantly.

### Feel and Style
- Clear pixel-art visuals with crisp scaling.
- Fast response to click input.
- Readable text at all resolutions.
- Stable frame pacing (target 60 FPS).

---

## 3) Technical Architecture

## 3.1 Modules
- `GameApp`
  - Program entry, window lifecycle, scene switching.
- `GameScene`
  - Owns board, HUD, input routing, update/render loop.
- `BoardSystem`
  - Holds card model instances and placement metadata.
- `CardEntity`
  - Individual card state, animation progress, rendering data.
- `MatchSystem`
  - Pair selection, compare logic, mismatch delay timing.
- `UISystem`
  - New Game button, timer text, move counter, end-game banner.
- `LayoutSystem`
  - Calculates responsive rectangles/sizes for all UI and cards.
- `AssetManager`
  - Texture/font loading with caching and fail-safe handling.
- `GameStats`
  - Move count, elapsed time, matched pairs.

## 3.2 Data Model
- `enum class CardState { FaceDown, FlippingToFront, FaceUp, FlippingToBack, Matched, Removed, Locked };`
- `struct CardData {`
  - `std::string id;` (stable unique id, ex: `luke_01`)
  - `std::string characterName;`
  - `std::string textureKeyFront;`
  - `CardState state;`
  - `bool selectable;`
  - `float flipProgress;` (`0.0f -> 1.0f`)
  - `sf::FloatRect bounds;`
  - `};`
- `struct MatchSelection { int firstIndex; int secondIndex; bool pendingResolution; sf::Time revealRemaining; };`
- `struct GameStats { int moves; int matchedPairs; sf::Time elapsed; bool timerRunning; };`

## 3.3 Main Loop Responsibilities
- Process window/system events.
- Handle input only when board is not locked.
- Update timers and animation progress using `deltaTime`.
- Resolve delayed pair comparison when reveal timer expires.
- Render background, board, cards, HUD in stable order.

---

## 4) State Machine Design

### Game-Level States
- `Boot`
- `Ready` (new shuffled board, waiting first click)
- `Playing`
- `ResolvingPair` (input lock while 2s reveal countdown runs)
- `Won` (game complete, waiting for New Game)

### Card-Level States
- `FaceDown`
- `FlippingToFront`
- `FaceUp`
- `FlippingToBack`
- `Matched`
- `Removed`

### Transition Rules
- Click allowed only for cards in `FaceDown`.
- During `ResolvingPair`, all card clicks ignored.
- A match moves both cards to `Matched`, then to `Removed` after a short removal transition.
- A mismatch moves both cards through `FlippingToBack` -> `FaceDown`.
- Win condition: `matchedPairs == 16`.

---

## 5) Grid, Coordinate System, and Responsive Full-Screen Layout

## 5.1 Virtual Reference Resolution
- Define logical design resolution: `1920x1080`.
- All UI sizes first computed in virtual units.
- Final on-screen positions scaled by:
  - `scale = min(windowWidth / 1920.0f, windowHeight / 1080.0f)`

## 5.2 Safe Area and Letterboxing Logic
- Compute playable region centered on screen:
  - `playW = 1920 * scale`
  - `playH = 1080 * scale`
  - `offsetX = (windowWidth - playW) / 2`
  - `offsetY = (windowHeight - playH) / 2`
- Keeps proportions identical regardless of monitor aspect ratio.

## 5.3 Board and HUD Allocation
- Top HUD area: ~18% of virtual height.
- Grid area: ~82% of virtual height.
- Grid uses 8 columns x 4 rows with fixed gaps.
- Grid slot positions remain fixed for the full match so removed pairs leave intentional empty slots.

### Example formula
- `gridPadding = 32 * scale`
- `gridGap = 16 * scale`
- `cardW = (gridWidth - (7 * gridGap)) / 8`
- `cardH = (gridHeight - (3 * gridGap)) / 4`
- Clamp card size to preserve designed aspect ratio (ex: 3:4).

## 5.4 Dynamic Typography
- Base font sizes in virtual units:
  - Title: 48
  - Stats: 36
  - Button label: 32
- Render size = `round(baseSize * scale)`, clamped to readability minima.

## 5.5 Full-Screen Behavior
- Start in full-screen desktop mode.
- Recompute layout on:
  - initial load
  - resize event
  - display mode change

---

## 6) Card Interaction and Animation Plan

## 6.1 Input Handling
- Map mouse position to card bounds.
- Ignore click if:
  - card already matched
  - card already removed
  - card currently animating
  - card already face-up and selected
  - board lock active

## 6.2 Flip Animation
- Duration target: `0.22s` per flip.
- Technique:
  - Animate X-scale from `1.0 -> 0.0` (hide old face), swap texture at midpoint, then `0.0 -> 1.0`.
  - Optional pixel snap to keep crisp edges.
- Easing:
  - Use simple smoothstep or quadratic easing for less mechanical movement.

## 6.3 2-Second Reveal Window
- After second card flip completes:
  - set `revealRemaining = 2.0s`
  - lock board input
- On timer expiry:
  - compare character ids
  - match: set both to `Matched`, play short vanish animation, then set to `Removed`
  - mismatch: trigger flip-back animation for both
  - unlock input when resolution animation finishes

## 6.4 Move Counter Rules
- Increment exactly once when second card is chosen.
- Never increment on first card click.

## 6.5 Timer Rules
- Start on first successful flip in `Ready`.
- Pause/freeze on `Won`.
- Display format: `MM:SS` (or `HH:MM:SS` if needed).

---

## 7) Asset Pipeline (Download + PNG Pixel Conversion)

## 7.1 Directory Convention
- `assets/source/` : downloaded originals.
- `assets/processed/` : standardized card portraits (PNG).
- `assets/ui/` : card back, button atlas, icons.
- `tools/` : automation scripts.
- `assets/manifest/cards.json` : mapping character -> output file.

## 7.2 Source Download Script
- Script file: `tools/fetch_assets.py`.
- Input: `assets/manifest/sources.json` (character names + image URLs).
- Behavior:
  - create `assets/source/` if missing
  - download each file with retries and timeout
  - validate content type and minimum dimensions
  - store deterministic filenames (`luke_skywalker.jpg`, etc.)
  - emit download report (success/failure list)

## 7.3 Pixel-Art Conversion Script
- Script file: `tools/process_assets.py`.
- Convert originals to consistent card faces:
  - crop to portrait ratio
  - resize down (ex: 128x128 or 192x192) with nearest-neighbor
  - optional limited palette quantization
  - resize up to final display source size using nearest-neighbor
  - export PNG to `assets/processed/`
  - generate a contact sheet preview for QA

### Suggested CLI tools
- Preferred: Python + Pillow for portability.
- Optional fallback: ImageMagick command wrappers.

## 7.4 Quality Gates for Assets
- Every character has exactly one final PNG.
- Visual framing consistent across all 16 characters.
- No transparent artifacts or stretched faces.
- Naming exactly matches manifest keys used by code.

## 7.5 Legal/Licensing Checklist
- Keep `assets/manifest/sources.json` with attribution/source links.
- Add `ASSETS_LICENSE.md` describing usage rights and restrictions.
- Ensure downloadable sources are legally usable for the project context.

---

## 8) Build/Project Structure Plan

```text
MemoryGame/
  CMakeLists.txt
  README.md
  DETAILED_PLAN.md
  src/
    main.cpp
    GameApp.cpp/.hpp
    GameScene.cpp/.hpp
    systems/
      BoardSystem.cpp/.hpp
      MatchSystem.cpp/.hpp
      UISystem.cpp/.hpp
      LayoutSystem.cpp/.hpp
    entities/
      CardEntity.cpp/.hpp
    core/
      AssetManager.cpp/.hpp
      GameStats.hpp
      Types.hpp
  assets/
    manifest/
      cards.json
      sources.json
    source/
    processed/
    ui/
  tools/
    fetch_assets.py
    process_assets.py
  tests/
    layout_tests.cpp
    match_logic_tests.cpp
```

---

## 9) Development Milestones

## Milestone 1 - Foundation
- Initialize CMake project and SFML 3.x linkage.
- Create window + fixed update/render loop.
- Implement full-screen startup.
- Add basic scene class and asset manager stubs.
- Acceptance criteria:
  - App launches on macOS and Windows.
  - Build succeeds with no runtime missing dependency errors.

## Milestone 2 - Board and Card Core
- Implement 32-card data generation (16 pairs).
- Shuffle and place into 8x4 grid.
- Render card backs only.
- Add clickable hit-testing.
- Acceptance criteria:
  - Grid is centered and responsive.
  - Every new game produces valid random pair distribution.

## Milestone 3 - Matching Logic + Timing
- Implement single/pair selection rules.
- Add 2-second reveal delay and mismatch flip-back.
- Add matched-pair removal flow (matched -> removed).
- Add move counter and timer.
- Acceptance criteria:
  - No illegal third-click during resolution.
  - Matched pairs are no longer clickable or rendered after removal.
  - Moves and timer values are accurate across full game.

## Milestone 4 - Animation + UI Polish
- Add flip animation with texture swap at midpoint.
- Add HUD (timer, moves, New Game button).
- Add win banner and restart path.
- Acceptance criteria:
  - Animation is smooth and deterministic.
  - UI remains proportional at multiple resolutions.

## Milestone 5 - Asset Pipeline Integration
- Build download + conversion scripts.
- Generate final card PNG set.
- Hook texture loading via manifest.
- Acceptance criteria:
  - One command refreshes all card assets.
  - Missing/corrupt asset errors are clearly reported.

## Milestone 6 - Cross-Platform Hardening
- Validate macOS and Windows builds.
- Add packaging notes and contribution workflow.
- Acceptance criteria:
  - Fresh clone setup works on both platforms.
  - README steps are tested end-to-end.

---

## 10) Test Strategy

### Unit-Level
- Pair resolution logic:
  - match path
  - match removal path
  - mismatch path
  - move increment timing
- Timer formatting function.
- Layout calculations for several aspect ratios.

### Integration-Level
- Full playthrough:
  - start game
  - complete all matches
  - verify matched pairs are removed from render/input path
  - verify final stats
- New Game reset:
  - clears board states
  - resets timer/moves
  - reshuffles cards

### Visual/Manual QA
- Resolutions: 1280x720, 1920x1080, 2560x1440, ultrawide.
- Confirm no overlapping UI.
- Confirm removed cards leave clean empty slots with no stale textures/hitboxes.
- Confirm pixel-art crispness (no blur from filtering).

---

## 11) Risks and Mitigations

- Risk: Inconsistent SFML 3 setup across platforms.
  - Mitigation: CMake presets + explicit dependency docs + CI build checks.
- Risk: Asset licensing uncertainty.
  - Mitigation: track source and license metadata per file.
- Risk: Responsive layout regressions on unusual aspect ratios.
  - Mitigation: automated layout tests + hard clamps and safe area logic.
- Risk: Animation/timing race conditions.
  - Mitigation: strict game state machine and input lock guards.

---

## 12) Definition of Done

- Game meets all listed requirements.
- 8x4 grid with 16 Star Wars character pairs implemented.
- Flip animation and exact 2-second pair reveal behavior implemented.
- Matched pairs disappear cleanly and are removed from input/render handling.
- Full-screen responsive UI is stable across macOS and Windows.
- Pixel-art card asset pipeline is scriptable and reproducible.
- README enables clean setup/build/contribution on both platforms.
- No critical gameplay bugs in standard playthrough testing.
