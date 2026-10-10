# PokketSX

<!-- Write the introduction here. -->

The README that PokketSX inherited from DuckStation, with its features, system requirements, build instructions and default bindings, is kept in [docs/legacy-readme.md](docs/legacy-readme.md) until this README replaces it.

---

## Licence and attribution

PokketSX is free software, licensed under the [GNU General Public License v3.0](LICENSE).

### Based on DuckStation

PokketSX is a modified version of [DuckStation](https://github.com/stenzek/duckstation) by stenzek and [its contributors](CONTRIBUTORS.md). It is a hard fork of the last DuckStation commit in which every file was available under GPL-3.0: `14ab642ff` in DuckStation's current history, which is `a5bf1ba5c` in its original history (see [CONTRIBUTING.md](CONTRIBUTING.md)). Their copyright notices are kept in the source files, and files changed for PokketSX carry an additional copyright line.

PokketSX is not affiliated with or supported by the DuckStation project, and is not kept compatible with it. Please report PokketSX problems in this repository's [issue tracker](https://github.com/mentalfoundry/pokketSX/issues), not upstream.

### PocketStation emulation

PocketStation support uses the [pokketstation](https://github.com/mentalfoundry/pokketstation) emulator core, licensed under the MIT License. A copy is included in [dep/pokketstation](dep/pokketstation).

### Third-party components

PokketSX includes code from other open-source projects under their own licenses. Their notices are in [data/resources/thirdparty.html](data/resources/thirdparty.html), which the app shows under **Help > View Third-Party Notices**. Dependency archives kept in this repository are listed with their licenses in [dep/archives/README.md](dep/archives/README.md). The Japanese, Korean and Chinese fonts that PokketSX downloads when needed are Noto Sans CJK, licensed under the SIL Open Font License 1.1 (see [dep/fonts](dep/fonts)).

### BIOS images

PokketSX does not include a PlayStation BIOS or a PocketStation BIOS. You need dumps from hardware that you own.

### Trademarks

"PlayStation", "PSX" and "PocketStation" are trademarks or registered trademarks of Sony Interactive Entertainment. PokketSX is not affiliated with or endorsed by Sony Interactive Entertainment.