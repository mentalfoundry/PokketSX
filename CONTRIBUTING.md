# Contributing to PokketSX

## Licensing

PokketSX is a hard fork of DuckStation at commit `14ab642ff` ("Common: Add
aligned_unique_ptr"), the last commit in which every file was available under
GPL-3.0. PokketSX is licensed under GPL-3.0 only.

DuckStation's history was rewritten after its license change, so this commit
has two IDs. `14ab642ff` is its ID in the current upstream history. In the
original history, as published on 1 September 2024, it is `a5bf1ba5c`. The two
differ only in the wording of their license headers (both offer GPL-3.0) and in
a few vector helper functions. `a5bf1ba5c` is the commit preserved by
[Leah Rowe's archive](https://codeberg.org/vimuser/duckstation), and the base of
[QuackStation](https://github.com/cameronbosch/quackstation).

DuckStation changes made after the fork point are **not** open source. Do not
bring any of them into PokketSX, by any route:

- no merges, rebases or cherry-picks from upstream DuckStation, or from any
  fork that includes its changes after the fork point;
- no copying code by hand from newer DuckStation sources;
- do not add upstream DuckStation as a git remote.

If a later upstream fix is needed, write it independently from a description
of the bug, not from upstream's code.

Other GPL forks of the same commit, such as QuackStation, are compatible with
PokketSX. Their changes can be brought in when they are that project's own work
under a GPL-compatible license, and not copied from DuckStation after the fork
point. Check where a change came from before bringing it in.

## File headers

Most source files inherited from DuckStation carry a dual license header,
`(GPL-3.0 OR CC-BY-NC-ND-4.0)` or `(GPL-3.0 OR PolyForm-Strict-1.0.0)`.
PokketSX uses every such file under its GPL-3.0 option. Leave those headers
unchanged in files you have not modified.

Never remove or alter an existing copyright line. When you modify a file, add
your own copyright line below the existing ones and, if the file has a dual
license header, change it to GPL-3.0 (a modified file can only be distributed
under that option):

```cpp
// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-FileCopyrightText: 2026 Your Name
// SPDX-License-Identifier: GPL-3.0
```

New files carry your copyright line and `GPL-3.0`. Third-party code keeps its
own license header.

## License check

`scripts/check-license.sh` fails if upstream history after the fork point is
present, or if any commit since the fork point has an SPDX license expression
that does not allow use under GPL-3.0. Expressions are checked against an
allow-list of GPL-compatible licenses, so an unknown license fails whatever its
name. CI runs it on every push and pull request. To run it before every push
locally:

```
git config core.hooksPath scripts/hooks
```
## Changelog

The rolling release and the update dialog of the application show a list of
changes. The list comes from the `Changelog:` trailers in the commit messages.
`scripts/release-notes.sh` writes the release notes from these trailers.

If users can see the effect of a commit, add a `Changelog:` trailer to the
commit message:

- Put the trailer in the last paragraph of the message. Do not put other text
  in that paragraph.
- Write one change in each trailer. A commit can have more than one trailer.
- Write the change for users, in ASD-STE100 Simplified Technical English.
- To continue a long change on the next line, start that line with a space.

Example:

```
PocketStation: Make the device part of save states

Loading a state left the device as it was, so a game that was talking to it
carried on with a device from a different point in time.

Changelog: Save states now include the PocketStation.
Changelog: Rewind and runahead now operate when a PocketStation is in use.
```

Do not add a trailer to a commit if users cannot see its effect, for example a
change to CI or to the regression tests.

If a commit makes older save states incompatible, put `[SAVEVERSION+]` in its
title. If a commit resets the settings, put `[SETTINGSVERSION+]` in its title.
The update dialog then shows a warning.
