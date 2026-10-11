# PokketSX

PokketSX is a hard fork of a PS1 emulator with the express purpose of providing the best PS1 experience for PocketStation fans.

If you are wanting the best pure PS1 experience, allow me to instead suggest trying the latest version of [DuckStation](https://www.duckstation.org/) - it is what you are after, it is really good.

If you are still here, it means you are interested in having a well running PS1 emulator that has best in class PocketStation support. 

If that sounds like you, I ask only that you read and follow the golden rule below before you download the emulator and enjoy the functionality PokketSX offers. 

**Please do not harass / beg the upstream developers of DuckStation for PocketStation support.** They will implement PocketStation support if they so choose and at their own pace. All issues with PokketSX should be raised in this repo alone. 

We are already a niche group of hobbyists - be excellent to each other.

---

## Project Status
The Pocketstation to PS1 communication code is functional. Looking to get a release build up soon.

---

## How to use
There is an additional setting on the "BIOS" tab that takes a PockeStation retail BIOS. On the "Memory Cards" tab there is an additional checkbox for each memory card which lets you toggle if a PocketStation should replace the normal memory card.

The README that PokketSX inherited from DuckStation, with its features, system requirements, build instructions and default bindings, is kept here [docs/legacy-readme.md](docs/legacy-readme.md) for your easy reference in case you need it. Note it looks pretty similar to the current DuckStation readme but will likely contain subtle differences so read it carefully!

---

## FAQ

Below are my answers to questions that I expect you will ask.

<details>
<summary>Rationale for this emulator</summary>

I aim to offer PocketStation fans a PocketStation supported PS1 emulator that is easy to use and runs great. The last open source version of DuckStation is the best choice as a starting point for the following reasons:
- The PS1 core was already excellent - adding functional PocketStation support to the core is really the only thing that is missing for PocketStation fans.  
- The bells and whistles added since DuckStation has become non open source are not materially important to what most of us are here for; to easily and correctly read and write to PS1 save files with PocketStation data for the titles that support the PocketStation (and then play the PS1 game).
</details>

#

<details>
<summary>Why the hard fork?</summary>

For some time now DuckStation has been under a source available (not open source) license, so newer DuckStation code cannot be merged into PokketSX. Beyond the licensing, the goals and methods of these two repos are not mutually compatible. 

- DuckStation's focus is clearly on the very best PS1 emulation experience, especially on a technical level.

- PokketSX's focus is on providing the best PocketStation emulation feature set on top of an already very good PS1 core.

- AI usage philosophy, the maintainers of both repos have differing opinions on AI usage in coding. This repo's AI usage is [described here](#on-ai-usage).
</details>

#
<details>
<summary>Isn't PokketSX going to be obsolete once DuckStation supports the PocketStation?</summary>

I actually don't think so for the reasons outlined in why this is a hard fork above. DuckStation will always focus on PS1 first (as it should). There are options and features that PocketStation users will want that are unlikely to ever be adopted or implemented into DuckStation. These are features that PokketSX will not shy away from.
</details>

#
<details>
<summary>What if a PS1 PocketStation title has technical issues?</summary>

I will readily admit that I am easily outclassed in my understanding of the PS1 - it is not my background nor passion (I just think PocketStations are neat!). But with the advent of AI what I lack in deep understanding of the PS1 I should still be able to step in and fix things without everything breaking down into spaghetti. Please raise an Issue and I'll endeavour to fix it if a fix is needed.
</details>

#
<details>
<summary>Is this allowed?</summary>

Yes. DuckStation's code up to that commit is GPL-3.0, which allows anyone to fork and modify it. This repo is based on the last open source commit for DuckStation (see [Licence and attribution](#licence-and-attribution)) and will not be striving for feature parity with the modern DuckStation. If those features really matter to you then you should be using DuckStation (again, please don't go asking for PocketStation support in DuckStation!).
</details>

---

## On AI usage

This repository uses AI assistance in the development of its code. The AI is always human steered using my best judgement and the application of my software development experiences and background. Thankfully there is also plenty of open source PS1 emulation information out there to draw on but honestly the PS1 core as it stands is already pretty darn good. Any changes I make will likely be very surgical and should be easy to scrutinise. 

I'll endeavour not to serve you spaghetti. Hope you enjoy the sauce.

---

## Licence and attribution

PokketSX is free software, licensed under the [GNU General Public License v3.0 only](LICENSE).

### Based on DuckStation

PokketSX is a modified version of [DuckStation](https://github.com/stenzek/duckstation) by stenzek and [its contributors](CONTRIBUTORS.md). It is a hard fork of the last DuckStation commit in which every file was available under GPL-3.0: `14ab642ff` in DuckStation's current history, which is `a5bf1ba5c` in its original history (see [CONTRIBUTING.md](CONTRIBUTING.md)). Their copyright notices are kept in the source files, and files changed for PokketSX carry an additional copyright line.

PokketSX is not affiliated with or supported by the DuckStation project, and is not kept compatible with it. Please report PokketSX problems in this repository's [issue tracker](https://github.com/mentalfoundry/PokketSX/issues), not upstream.

### PocketStation emulation

PocketStation support uses the [pokketstation](https://github.com/mentalfoundry/pokketstation) emulator core, licensed under the MIT License. A copy is included in [dep/pokketstation](dep/pokketstation).

### Third-party components

PokketSX includes code from other open-source projects under their own licenses. Their notices are in [data/resources/thirdparty.html](data/resources/thirdparty.html), which the app shows under **Help > View Third-Party Notices**. Dependency archives kept in this repository are listed with their licenses in [dep/archives/README.md](dep/archives/README.md). The Japanese, Korean and Chinese fonts that PokketSX downloads when needed are Noto Sans CJK, licensed under the SIL Open Font License 1.1 (see [dep/fonts](dep/fonts)).

### BIOS images

PokketSX does not include a PlayStation BIOS or a PocketStation BIOS. You need dumps from hardware that you own.

### Trademarks

"PlayStation", "PSX" and "PocketStation" are trademarks or registered trademarks of Sony Interactive Entertainment. PokketSX is not affiliated with or endorsed by Sony Interactive Entertainment.
