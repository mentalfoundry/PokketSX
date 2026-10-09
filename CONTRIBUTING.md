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

Keep existing copyright and license headers intact. New files should carry a
GPL-3.0 SPDX header.

## License check

`scripts/check-license.sh` fails if any commit since the fork point carries a
non-commercial Creative Commons license header, or if upstream history after the
fork point is present. CI runs it on every push and pull request. To run it
before every push locally:

```
git config core.hooksPath scripts/hooks
```