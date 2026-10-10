# Vendored dependency archives

These source archives were previously downloaded from repositories hosted by stenzek (the upstream author). They are kept here so PokketSX builds do not depend on those repositories staying online; `stenzek/lunasvg` has already been deleted.

The build scripts (`scripts/deps/*`) and Flatpak modules (`scripts/flatpak/modules/*`) copy these files instead of downloading them, and still verify each one against its SHA-256. Each file is byte-identical to the archive GitHub served for the pinned commit, so the checksums are unchanged.

| Archive | Pinned commit | Originally fetched from | Upstream project | Licence |
|---|---|---|---|---|
| `cpuinfo-*.tar.gz`, `cpuinfo-*.zip` | `7524ad50` | stenzek/cpuinfo (tar.gz), pytorch/cpuinfo (zip) | pytorch/cpuinfo | BSD-2-Clause |
| `discord-rpc-*` | `144f3a3f` | stenzek/discord-rpc | discord/discord-rpc | MIT |
| `lunasvg-*` | `9af1ac7b` | stenzek/lunasvg (deleted; recovered via sammycage/lunasvg) | sammycage/lunasvg | MIT |
| `shaderc-*` | `f60bb80e` | stenzek/shaderc | google/shaderc | Apache-2.0; bundles glslang (BSD-3-Clause, BSD-2-Clause, MIT, Apache-2.0), SPIRV-Tools (Apache-2.0), SPIRV-Headers (MIT-style) |
| `soundtouch-*` | `463ade38` | stenzek/soundtouch | SoundTouch | LGPL-2.1 (built and shipped as a shared library) |

Each archive contains its project's full licence text.

The AppImage packaging tools (linuxdeploy, linuxdeploy-plugin-qt, the type2 runtime) are not vendored. They are prebuilt binaries that bundle GPL and LGPL components, so redistributing them would require providing their corresponding source. `scripts/appimage/make-appimage.sh` downloads them from their upstream projects instead.