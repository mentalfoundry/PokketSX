> **Legacy README.** This was PokketSX's README until October 2026. It is the README of DuckStation's September 2024 version, adapted for PokketSX's name, paths and build steps, and is kept for reference. Some of it may be out of date. See the main [README](../README.md) for current information.

# PokketSX - PlayStation 1 Emulator

[What to Expect](#what-to-expect) | [Features](#features) | [Building](#building) | [User Directories](#user-directories) | [Licence and Credits](#licence-and-credits)

PokketSX is a simulator/emulator of the Sony PlayStation(TM) console, aiming to add PocketStation support. It is a hard fork of [DuckStation](https://github.com/stenzek/duckstation) at commit `14ab642ff`, the last commit in which all of DuckStation's code was available under the GPL.

**Status:** PocketStation support is in development and is not in this repository yet. There are no PokketSX releases yet; when there are, they will be on the [Releases](https://github.com/mentalfoundry/PokketSX/releases) page.

PokketSX is not affiliated with or supported by the DuckStation project, and is not kept compatible with it: settings, save states, the PINE socket (`pokketsx.sock`) and app identifiers are all separate. Please report PokketSX problems in this repository's [issue tracker](https://github.com/mentalfoundry/PokketSX/issues), not upstream.

A PS1 or PS2 "BIOS" ROM image is required to start the emulator and to play games. You can use an image from any hardware version or region, although mismatching game regions and BIOS regions may have compatibility issues. A ROM image is not provided with the emulator for legal reasons; you should dump this from your own console using Caetla or other means.

## What to expect

PokketSX is built on DuckStation's September 2024 code and focuses on PocketStation support. It does not follow newer DuckStation releases, so compared with current DuckStation:

- Some games may behave differently, run slower, or need settings adjusted by hand. Newer emulation fixes and per-game settings are not included.
- Cheats, save states, shaders and settings from newer DuckStation versions may not work. Only load save states from sources you trust.
- Newer interface features, graphics options, tools and controller types are not available.
- Controller input uses SDL2, so very recent controllers may be less well supported.

PokketSX still runs on macOS 11 and later.

## Features

PokketSX has a full desktop frontend built using Qt, as well as a fullscreen/TV UI based on Dear ImGui. Features include:

 - CPU Recompiler/JIT (x86-64, armv7/AArch32, AArch64, RISC-V/RV64).
 - Hardware (D3D11, D3D12, OpenGL, Vulkan, Metal) and software rendering.
 - Upscaling, texture filtering, and true colour (24-bit) in hardware renderers.
 - Accurate blending via Rasterizer Order Views/Fragment Shader Interlock.
 - PGXP for geometry precision, texture correction, and depth buffer emulation.
 - Motion adaptive deinterlacing.
 - Adaptive downsampling filter.
 - Screen rotation for vertical or "TATE" shmup games.
 - Post processing shader chains (GLSL and Reshade FX).
 - "Fast boot" for skipping BIOS splash/intro.
 - Save state support, with runahead and rewind.
 - Windows, Linux, macOS support.
 - Supports reading directly from CD, bin/cue images, raw bin/img files, MAME CHD, single-track ECM, MDS/MDF, and unencrypted PBP formats.
 - Preloading of disc images to RAM to avoid disk sleeping hitches.
 - Automatic loading/applying of PPF patches.
 - Direct booting of homebrew executables.
 - Direct loading of Portable Sound Format (psf) files.
 - Time stretched audio when running outside of 100% speed, and surround sound expansion.
 - Digital and analog controllers for input (rumble is forwarded to host).
 - GunCon and Justifier lightgun support (simulated with mouse).
 - NeGcon support.
 - Automatic content scanning - game titles/hashes are provided by redump.org.
 - Optional automatic switching of memory cards for each game.
 - Supports loading cheats from existing lists.
 - Memory card editor and save importer.
 - Emulated CPU overclocking.
 - Integrated and remote debugging.
 - Multitap controllers (up to 8 devices).
 - RetroAchievements.
 - Video capture with Media Foundation (Windows) and [FFmpeg](https://www.ffmpeg.org/) (All Platforms) backends.

## System Requirements
 - An x86_64 CPU with SSE4.1 (any CPU made after 2007), or an AArch32/armv7, AArch64/ARMv8, or RISC-V/RV64 CPU.
 - For the hardware renderers, a GPU capable of OpenGL 3.1/OpenGL ES 3.1/Direct3D 11 Feature Level 10.0 (or Vulkan 1.0) and above.
 - SDL, XInput or DInput compatible game controller (e.g. XB360/XBOne/XBSeries). DualShock 3 users on Windows will need to install the official DualShock 3 drivers included as part of PlayStation Now.

### LibCrypt protection and SBI files

A number of PAL region games use LibCrypt protection, requiring additional CD subchannel information to run properly. LibCrypt not functioning usually manifests as hanging or crashing, but can sometimes affect gameplay too, depending on how the game implemented it.

For these games, make sure that the CD image and its corresponding SBI (.sbi) file have the same name and are placed in the same directory. PokketSX will automatically load the SBI file when it is found next to the CD image.

For example, if your disc image was named `Spyro3.cue`, you would place the SBI file in the same directory, and name it `Spyro3.sbi`.

CHD images with built-in subchannel information are also supported.

## Building

### Windows
Requirements:
 - Visual Studio 2022 (v143 toolset)

1. Clone the repository: `git clone https://github.com/mentalfoundry/PokketSX.git`.
2. Build the dependencies: from the `scripts\deps` directory, run `build-dependencies-windows-x64.bat`. It sets up the Visual Studio environment itself and installs to `dep\msvc\deps-x64`. This will take a while.
3. Open the Visual Studio solution `pokketsx.sln` in the root, or "Open Folder" for a CMake build.
4. Build the solution.
5. Binaries are located in `bin/x64`.
6. Run `pokketsx-qt-x64-Release-MSVC.exe`, or the executable for whichever configuration you built.

### Linux

#### Required Dependencies

Ubuntu/Debian package names:
```
build-essential clang cmake curl extra-cmake-modules git libasound2-dev libcurl4-openssl-dev libdbus-1-dev libdecor-0-dev libegl-dev libevdev-dev libfontconfig-dev libfreetype-dev libgtk-3-dev libgudev-1.0-dev libharfbuzz-dev libinput-dev libopengl-dev libpipewire-0.3-dev libpulse-dev libssl-dev libudev-dev libwayland-dev libx11-dev libx11-xcb-dev libxcb1-dev libxcb-composite0-dev libxcb-cursor-dev libxcb-damage0-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-present-dev libxcb-randr0-dev libxcb-render0-dev libxcb-render-util0-dev libxcb-shape0-dev libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev libxcb-xinput-dev libxcb-xkb-dev libxext-dev libxkbcommon-x11-dev libxrandr-dev lld llvm nasm ninja-build pkg-config zlib1g-dev
```

Fedora package names:
```
alsa-lib-devel brotli-devel clang cmake dbus-devel egl-wayland-devel extra-cmake-modules fontconfig-devel gcc-c++ gtk3-devel libavcodec-free-devel libavformat-free-devel libavutil-free-devel libcurl-devel libdecor-devel libevdev-devel libICE-devel libinput-devel libSM-devel libswresample-free-devel libswscale-free-devel libX11-devel libXau-devel libxcb-devel libXcomposite-devel libXcursor-devel libXext-devel libXfixes-devel libXft-devel libXi-devel libxkbcommon-devel libxkbcommon-x11-devel libXpresent-devel libXrandr-devel libXrender-devel lld llvm make mesa-libEGL-devel mesa-libGL-devel nasm ninja-build openssl-devel patch pcre2-devel perl-Digest-SHA pipewire-devel pulseaudio-libs-devel systemd-devel wayland-devel xcb-util-cursor-devel xcb-util-devel xcb-util-errors-devel xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel xcb-util-xrm-devel zlib-devel
```

#### Building

1. Clone the repository: `git clone https://github.com/mentalfoundry/PokketSX.git`, `cd PokketSX`.
2. Build dependencies. You can save these outside of the tree if you like. This will take a while. `scripts/deps/build-dependencies-linux.sh deps`.
3. Run CMake to configure the build system. Assuming a build subdirectory of `build-release`, run `cmake -B build-release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXE_LINKER_FLAGS_INIT="-fuse-ld=lld" -DCMAKE_MODULE_LINKER_FLAGS_INIT="-fuse-ld=lld" -DCMAKE_SHARED_LINKER_FLAGS_INIT="-fuse-ld=lld" -DCMAKE_PREFIX_PATH="$PWD/deps" -G Ninja`. If you want a release (optimized) build, include `-DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`.
4. Compile the source code. For the example above, run `ninja -C build-release`.
5. Run the binary, located in the build directory under `./build-release/bin/pokketsx-qt`.

### macOS

Requirements:
 - CMake
 - Xcode

1. Clone the repository: `git clone https://github.com/mentalfoundry/PokketSX.git`.
2. Build the dependencies. This will take a while. `scripts/deps/build-dependencies-mac.sh deps`.
3. Run CMake to configure the build system: `cmake -Bbuild-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DCMAKE_PREFIX_PATH="$PWD/deps"`.
4. Compile the source code: `cmake --build build-release --parallel`.
5. Run the binary, located in the build directory under `bin/PokketSX.app`.

## User Directories
The "User Directory" is where you should place your BIOS images, where settings are saved to, and memory cards/save states are saved by default.
An optional [SDL game controller database file](#sdl-game-controller-database) can be also placed here.

This is located in the following places depending on the platform you're using:

- Windows: My Documents\PokketSX
- Linux: `$XDG_CONFIG_HOME/pokketsx`, or `~/.local/share/pokketsx`.
- macOS: `~/Library/Application Support/PokketSX`.

So, if you were using Linux, you would place your BIOS images in `~/.local/share/pokketsx/bios`. This directory will be created upon running PokketSX for the first time.

PokketSX keeps its own settings, memory cards and save states here, separate from any other emulator. To bring existing ones over, copy them into this directory.

If you wish to use a "portable" build, where the user directory is the same as where the executable is located, create an empty file named `portable.txt` in the same directory as the PokketSX executable.

## Bindings for Qt frontend
Your keyboard or game controller can be used to simulate a variety of PlayStation controllers. Controller input is supported through DInput, XInput, and SDL backends and can be changed through `Settings -> General Settings`.

To bind your input device, go to `Settings -> Controllers`. Each of the buttons/axes for the simulated controller will be listed, alongside the corresponding key/button on your device that it is currently bound to. To rebind, click the box next to the button/axis name, and press the key or button on your input device that you wish to bind to. When binding rumble, simply press any button on the controller you wish to send rumble to.

## SDL Game Controller Database
PokketSX ships with a database of game controller mappings for the SDL controller backend, courtesy of https://github.com/mdqinc/SDL_GameControllerDB. The included `gamecontrollerdb.txt` file can be found in the `resources` subdirectory of the PokketSX program directory.

If you are experiencing issues binding your controller with the SDL controller backend, you may need to add a custom mapping to the database file. Make a copy of `gamecontrollerdb.txt` and place it in your [user directory](#user-directories) (or directly in the program directory, if running in portable mode) and then follow the instructions in the [SDL_GameControllerDB repository](https://github.com/mdqinc/SDL_GameControllerDB) for creating a new mapping. Add this mapping to the new copy of `gamecontrollerdb.txt` and your controller should then be recognized properly.

## Default bindings
Controller 1:
 - **Left Stick:** W/A/S/D
 - **Right Stick:** T/F/G/H
 - **D-Pad:** Up/Left/Down/Right
 - **Triangle/Square/Circle/Cross:** I/J/L/K
 - **L1/R1:** Q/E
 - **L2/R2:** 1/3
 - **L3/R3:** 2/4
 - **Start:** Enter
 - **Select:** Backspace

Hotkeys:
 - **Escape:** Open Pause Menu
 - **F11:** Toggle Fullscreen
 - **Tab:** Temporarily Disable Speed Limiter
 - **Space:** Pause/Resume Emulation

## Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md). In short: PokketSX must never take in DuckStation code from after the fork point, because that code is not open source.

## Licence and Credits

PokketSX is licensed under the [GNU General Public License v3.0](../LICENSE).

PokketSX is based on DuckStation by stenzek and [its contributors](../CONTRIBUTORS.md). Their copyright notices are preserved in the source.

"PlayStation" and "PSX" are registered trademarks of Sony Interactive Entertainment Europe Limited. This project is not affiliated in any way with Sony Interactive Entertainment.