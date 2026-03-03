# pemu (pfbneo) — PS Vita Fork

This is a fork of [Cpasjuste/pemu](https://github.com/Cpasjuste/pemu) with custom modifications for PS Vita, focused on the **pfbneo** (FinalBurn Neo) core.

Upstream: `https://github.com/Cpasjuste/pemu.git`
Fork: `https://github.com/cysin/pemu.git`

---

## What We Changed (vs Upstream)

### 1. RGUI Menu System (New)

Replaced the default pemu UI with a custom RetroArch-style RGUI overlay menu. All new files are in `src/cores/pfbneo/sources/`:

| File | Purpose |
|------|---------|
| `rgui_menu.cpp/h` | Base menu widget — item list with label/value, d-pad navigation, scrolling |
| `rgui_main.cpp/h` | Main menu controller — screen state machine, routes to submenus |
| `rgui_filebrowser.cpp/h` | ROM file browser with directory navigation |
| `rgui_state.cpp/h` | Save/load state slot menu |
| `rgui_cheats.cpp/h` | Cheats menu — wraps FBNeo `CheatInfo` linked list, cycles options via `CheatEnable()` |
| `rgui_turbo.cpp/h` | Turbo fire settings — per-player (2), per-button (8: A,B,C,D,X,Y,Z,L), configurable speed |

**Menu structure (in-game):**
- Resume Game
- Load ROM
- Save State / Load State
- Cheats
- Turbo Fire
- Settings (Show FPS, Scaling, Filtering, Audio, Neo-Geo BIOS, etc.)
- Quit

**Integration points:**
- `src/cores/main.cpp` — Creates `RguiMain` as overlay (`setLayer(100)`), hides old `UiRomList`
- `src/cores/pfbneo/sources/pfbneo_ui_emu.cpp` — Intercepts Select+Start combo to open RGUI instead of old menu; routes input to RGUI when visible

### 2. Turbo Fire (New)

Automatic rapid-fire for any button combination, per player.

- **Config**: `TurboConfig g_turbo` global — `enabled[2][8]` array + `speed` (frames between toggles, 1-10)
- **Processing**: In `retro_input.cpp` `InputMake()` — on turbo-off frames, clears the input value for turbo-enabled buttons that are held down
- **Button mapping**: Maps turbo button indices to `RETRO_DEVICE_ID_JOYPAD_*` constants, then finds matching `sKeyBinds`/`GameInp` entries to zero out

### 3. Cheats Support (New)

- Added `szAppCheatsPath` in `paths.cpp` — creates `<dataPath>/cheats/` directory
- `drv.cpp` `DrvInit()` — calls `ConfigCheatLoad()` after driver initialization, sets `bCheatsAllowed = true`
- `drv.cpp` `DrvExit()` — calls `CheatExit()` to clean up cheat state
- FBNeo submodule (`external/cores/FBNeo`) — added debug printf logging in `conc.cpp` `ConfigCheatLoad()` / `ConfigParseFile()`

### 4. Per-Game Settings Persistence (New)

Turbo fire and cheat selections are saved per game and auto-restored on next load.

**File format** — plain text in `<dataPath>/configs/`:
- `<driverName>_turbo.cfg`: `speed=N` + `player,button=1` lines
- `<driverName>_cheats.cfg`: `cheatIndex=selectedOption` lines (only non-default)

**Static methods** (callable without menu instance):
- `RguiTurbo::save(driverName)` / `RguiTurbo::load(driverName)`
- `RguiCheats::saveState(driverName)` / `RguiCheats::loadState(driverName)`

**Call sites:**
- **Load**: `drv.cpp` `DrvInit()` — after `ConfigCheatLoad()`, calls both `load` methods
- **Save**: `drv.cpp` `DrvExit()` — before `CheatExit()`, calls both `save` methods
- **Save**: `rgui_main.cpp` — when exiting cheats/turbo submenus (immediate feedback)

### 5. Build Fix

- `CMakeLists.txt` — removed `conc.cpp` from `REMOVE_ITEM` list (file doesn't have that name in current FBNeo)

### 6. Renderer Size Fix

- `main.cpp` — changed `PEMUUiMain(Vector2f{1280, 720})` to `Vector2f{0, 0}` (auto-detect)

---

## Key Globals & Externs

These are defined in `sources/fbneo/paths.cpp` with no header declarations. Use `extern char varname[];` locally when needed:

| Global | Path | Description |
|--------|------|-------------|
| `szAppHomePath` | `<dataPath>/` | Base data directory |
| `szAppRomPath` | `<dataPath>/arcade/` | ROM files |
| `szAppConfigPath` | `<dataPath>/configs/` | Per-game config files |
| `szAppEEPROMPath` | `<dataPath>/eeproms/` | EEPROM save states |
| `szAppCheatsPath` | `<dataPath>/cheats/` | Cheat definition files |
| `szAppSavePath` | `<dataPath>/saves/` | Save files |
| `szAppHiscorePath` | `<dataPath>/hiscores/` | High score data |

On Vita, `<dataPath>` = `ux0:/data/pfba/`.

Other important globals:
- `g_turbo` (`TurboConfig`) — turbo fire state, defined in `rgui_turbo.cpp`
- `g_rgui` (`RguiMain*`) — RGUI instance, defined in `main.cpp`
- `pemu_ui` (`PEMUUiMain*`) — main UI instance, defined in `main.cpp`
- `bDrvOkay` (`int`) — 1 when a driver is loaded, defined in `drv.cpp`
- `pCheatInfo` (`CheatInfo*`) — FBNeo cheat linked list head, defined in FBNeo `cheat.cpp`

---

## Build Guide

### Quick Build

```bash
cd /home/star/work/fbnoe_vita/pemu/cmake-build
make -j$(nproc) pfbneo.vpk
```

Output: `cmake-build/src/cores/pfbneo/pfbneo.vpk` (~15MB)

### First-Time / After FBNeo Changes

```bash
make pfbneo.deps              # Generate headers (ctv.h, driverlist.h, etc.)
make -j$(nproc) pfbneo.vpk
```

### Reconfigure From Scratch

```bash
cd /home/star/work/fbnoe_vita/pemu
rm -rf cmake-build && mkdir cmake-build && cd cmake-build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .. \
  -DPLATFORM_VITA=ON -DOPTION_MPV_PLAYER=OFF
make pfbneo.deps
make -j$(nproc) pfbneo.vpk
```

### Cross-Compilation Toolchain

- **SDK**: VitaSDK at `$VITASDK` (currently `/home/star/work/game_cheats/vitasdk`)
- **Compiler**: `arm-vita-eabi-gcc` / `arm-vita-eabi-g++`
- **Architecture**: ARMv7-A Cortex-A9, NEON SIMD, hard float
- **C++ standard**: C++17

### CMake Variables

| Variable | Value | Purpose |
|----------|-------|---------|
| `PLATFORM_VITA` | ON | Enables Vita cross-compilation & VPK target |
| `TITLE_ID` | PFBN00001 | Vita app identifier in param.sfo |
| `OPTION_LIGHT` | OFF | Set ON to strip console drivers (smaller binary) |
| `OPTION_MPV_PLAYER` | OFF | Disable MPV (not available on Vita) |

---

## Project Layout

```
pemu/
├── CMakeLists.txt                         # Root project (v7.1)
├── CLAUDE.md                              # This file
├── cmake-build/                           # Pre-configured Vita build dir
│   └── src/cores/pfbneo/
│       ├── pfbneo.vpk                     # Built VPK package
│       └── deps/                          # Generated headers
├── src/
│   ├── cores/
│   │   ├── main.cpp                       # Entry point (modified: RGUI integration)
│   │   └── pfbneo/
│   │       ├── CMakeLists.txt             # pfbneo build config
│   │       ├── sources/                   # *** Main edit area ***
│   │       │   ├── rgui_main.cpp/h        # RGUI main menu controller
│   │       │   ├── rgui_menu.cpp/h        # Base menu widget
│   │       │   ├── rgui_turbo.cpp/h       # Turbo fire menu + save/load
│   │       │   ├── rgui_cheats.cpp/h      # Cheats menu + save/load
│   │       │   ├── rgui_filebrowser.cpp/h # ROM file browser
│   │       │   ├── rgui_state.cpp/h       # Save/load state menu
│   │       │   ├── pfbneo_ui_emu.cpp/h    # Emu UI (modified: RGUI routing)
│   │       │   ├── pfbneo_config.cpp/h    # Config handling
│   │       │   └── fbneo/                 # FBNeo integration
│   │       │       ├── drv.cpp            # DrvInit/DrvExit (modified: cheats, save/load)
│   │       │       ├── paths.cpp          # Path globals (modified: szAppCheatsPath)
│   │       │       └── retro_input.cpp    # Input (modified: turbo fire processing)
│   │       └── data/                      # Vita assets (icons, skins, livearea)
│   └── skeleton/                          # cross2dui shared UI framework
└── external/
    ├── cores/FBNeo/                       # FBNeo engine (submodule, modified)
    └── libcross2d/                        # Cross-platform lib (submodule, unmodified)
```

### Source Auto-Discovery

New `.cpp` files in `sources/` or `sources/fbneo/` are auto-picked up by CMake GLOB:
```cmake
file(GLOB SRC_PFBA sources/*.c* sources/fbneo/*.c*)
```
After adding new files, touch the CMakeLists.txt to trigger re-glob:
```bash
touch src/cores/pfbneo/CMakeLists.txt
```

### Include Paths

All of `sources/`, `sources/fbneo/`, and the FBNeo source tree are in the include path. Any header in these directories can be included with just `#include "filename.h"`.

---

## Source Groups Compiled Into pfbneo

| Group | Source | Description |
|-------|--------|-------------|
| SRC_PFBA | `sources/*.c*`, `sources/fbneo/*.c*` | Custom UI & integration (our code) |
| SRC_BURN | `FBNeo/src/burn/**/*.c*` | Emulation engine core |
| SRC_BURNER | `FBNeo/src/burner/*.c*` + selected SDL files | Burner abstraction |
| SRC_DRV | `FBNeo/src/burn/drv/<family>/*.cpp` | Arcade machine drivers (22 families) |
| SRC_CPU | `FBNeo/src/cpu/**/*.cpp` | CPU emulators (M68K via Cyclone ASM on Vita) |
| SRC_INTF | `FBNeo/src/intf/**/*.cpp` | Audio/input/CD interfaces |
| SRC_7Z | `FBNeo/src/dep/libs/lib7z/*.c` | 7-Zip decompression |

### Generated Dependencies (`make pfbneo.deps`)

Run on the **host** (not cross-compiled). Required before first build:

| Output | Generator | Description |
|--------|-----------|-------------|
| `deps/m68kops.h/.c` | `m68kmake` | M68K opcode tables |
| `deps/ctv.h` | `ctv_make.cpp` | Capcom CPS TV chip |
| `deps/pgm_sprite.h` | `pgm_sprite_create.cpp` | PGM sprite conversion |
| `deps/driverlist.h` | Perl script | Master driver registry |
| `deps/neo_sprite_func.h` | Perl script | Neo-Geo sprite functions |
| `deps/toa_gp9001_func.h` | Perl script | Toaplan GPU functions |
| `deps/cave_tile_func.h` | Perl script | Cave tile rendering |
| `deps/cave_sprite_func.h` | Perl script | Cave sprite rendering |
| `deps/psikyo_tile_func.h` | Perl script | Psikyo tile rendering |

### VPK Packaging Pipeline

The `pfbneo.vpk` make target runs:
1. `vita-elf-create` — ELF to VELF
2. `vita-make-fself -c` — VELF to eboot.bin (signed, compressed)
3. Copy `data_romfs/` (skins, hiscores) into `vpk/` directory
4. Copy `eboot.bin` into `vpk/`
5. `vita-mksfoex` — create `param.sfo` metadata
6. `zip -r pfbneo.vpk vpk/` — package as VPK (ZIP format)

### Linking

```cmake
target_link_libraries(pfbneo cross2dui cross2d ScePower_stub pthread)
```

- **cross2dui** — pemu UI framework (static lib)
- **cross2d** — libcross2d (static lib, transitively links vita2d, SceGxm_stub, etc.)
- **ScePower_stub** — Vita power management
- **pthread** — POSIX threads

---

## Vita-Specific Notes

- **Cyclone ASM**: M68K CPU emulation uses ARM-optimized JIT (`sources/cpu/cyclone/Cyclone.s`) instead of portable C `m68000_intf.cpp`
- **No SDL2 renderer**: Vita uses native `vita2d` (GXM API wrapper) via libcross2d platform layer
- **Vita data path**: `ux0:/data/pfba/` — ROMs in `arcade/`, configs in `configs/`, cheats in `cheats/`
- **Speedhacks**: `-DUSE_SPEEDHACKS` enabled for performance on Cortex-A9
- **NEON SIMD**: `-mfpu=neon` for vector operations

---

## Troubleshooting

**Missing generated headers** (ctv.h, driverlist.h):
```bash
make pfbneo.deps
```
Requires: `g++` (host), `perl`

**"arm-vita-eabi-gcc: not found"**:
```bash
export VITASDK=/home/star/work/game_cheats/vitasdk
export PATH=$VITASDK/bin:$PATH
```

**New source file not compiled**: Touch CMakeLists.txt to re-glob:
```bash
touch src/cores/pfbneo/CMakeLists.txt
```

**Stale objects after branch switch**:
```bash
cd cmake-build/src/cores/pfbneo
rm -f CMakeFiles/pfbneo.dir/sources/**/*.obj
```
