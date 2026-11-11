# PokketSX libretro core

The PokketSX libretro core is a version of PokketSX that operates in RetroArch and in other libretro frontends. The core file is `pokketsx_libretro.dll`.

The core is experimental. Some functions are not available yet. Refer to [Functions that are not available](#functions-that-are-not-available).

## Install the core on Windows

The rolling release has the file `pokketsx-libretro-windows-x64.zip`. This file has the same directory structure as a RetroArch installation:

| Directory in the archive | Contents |
|---|---|
| `cores` | The core and the DLL files that the core uses |
| `info` | The core information file |
| `system/pokketsx/resources` | The PokketSX resources |

To install the core, do these steps:

1. Extract `pokketsx-libretro-windows-x64.zip` into the RetroArch directory.
2. Put a PS1 BIOS file in the RetroArch `system` directory.
3. To use a PocketStation, put a PocketStation BIOS file in the `system` directory.
4. Start RetroArch.
5. Load a PS1 disc image with the PokketSX core.

The core finds a BIOS file from the contents of the file. The name of the file is not important.

> **CAUTION:** Keep the DLL files in the same directory as `pokketsx_libretro.dll`. Do not put them in the directory that contains `retroarch.exe`. RetroArch has different DLL files with the same names, for example `SDL2.dll`. The core loads its DLL files only from its own directory.

We tested the core with RetroArch 1.22.2.

## Directories that the core uses

| Data | Directory |
|---|---|
| PS1 BIOS files | The system directory of the frontend |
| PocketStation BIOS files | The system directory of the frontend |
| PokketSX resources | `pokketsx/resources` in the system directory |
| Memory cards | The save directory of the frontend |
| Other data, for example the cache | `pokketsx` in the system directory |

## Core options

| Option | Values | Function |
|---|---|---|
| Renderer | Software, Hardware | Hardware selects the best hardware renderer for the computer. The core reads each frame back from the GPU. |
| Internal Resolution | 1x to 8x | Sets the resolution of the hardware renderer. |
| Console Region | Auto, NTSC-U, NTSC-J, PAL | Sets the region of the console. The change occurs when the core starts the next game. |
| Fast Boot | disabled, enabled | When enabled, the console does not show the BIOS start sequence. |
| PocketStation in Memory Card Slot 1 | disabled, enabled | When enabled, a PocketStation replaces the memory card in slot 1. |

Each controller port can have one of these devices:

- PlayStation Controller (digital)
- DualShock
- None

## Save states

The core supports save states. You can save and load states from the RetroArch menu, and use the RetroArch rewind function.

A save state contains the full console, and also these items:

- The memory cards. When you load a state, the memory cards go back to their data at the time of the state.
- The PocketStation, with its flash memory. When you load a state, the PocketStation goes back to the time of the state, together with the console. A download to the PocketStation that was in progress continues correctly.
- The controller types. When you load a state, each port gets the controller type that it had at the time of the state.

> **CAUTION:** Make sure that you do not load an older state after you save your game to a memory card or to the PocketStation. The memory card or the PocketStation goes back to its data at the time of the state, and the save that you made after that time is lost.

A state from an older version of PokketSX does not contain the PocketStation. When you load such a state, the PocketStation continues from its current condition.

Runahead in RetroArch operates with this core, but it is slow. The core loads a state in approximately 20 ms, and runahead loads a state for each frame. Do not use runahead with this core yet.

## Build the core

### Windows

1. Open Git Bash in the PokketSX directory.
2. Enter these commands:

   ```bash
   export _CL_='//wd4996'
   "/c/Program Files/Microsoft Visual Studio/18/Professional/MSBuild/Current/Bin/MSBuild.exe" \
     src/pokketsx-libretro/pokketsx-libretro.vcxproj \
     -p:SolutionDir="$(pwd -W)/" -p:Configuration=Release -p:Platform=x64 \
     -m -v:minimal -noautoresponse
   ```

The build puts the core and its DLL files in `bin/x64/libretro-Release/`.

### CMake

Configure CMake with `-DBUILD_LIBRETRO_CORE=ON`.

## Functions that are not available

The core does not have these functions yet:

- Display of the PocketStation screen.
- Direct output to the video system of the frontend. The core reads each frame back from the GPU. This is slower at high resolutions.
- Disc change, cheats, achievements, multitap, light guns and mice.
- Builds for platforms other than Windows x64. On macOS, the recompiler does not operate in the core yet.
