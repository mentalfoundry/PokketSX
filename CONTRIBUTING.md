# Contributing to PokketSX

## Licensing

PokketSX is a hard fork of DuckStation at commit `14ab642ff` ("Common: Add
aligned_unique_ptr"), the last commit in which every file was available under
GPL-3.0. PokketSX is licensed under GPL-3.0 only.

DuckStation changes made after that commit are **not** open source. Do not
bring any of them into PokketSX, by any route:

- no merges, rebases or cherry-picks from upstream DuckStation or its forks;
- no copying code by hand from newer DuckStation sources;
- do not add upstream DuckStation as a git remote.

If a later upstream fix is needed, write it independently from a description
of the bug, not from upstream's code.

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