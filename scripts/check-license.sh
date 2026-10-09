#!/usr/bin/env bash
# Fails if a revision (default HEAD), or any commit between the GPL base and it,
# contains code that is not available under the GPL.
#
# PokketSX is a hard fork of the last DuckStation commit in which every file
# was available under GPL-3.0. Upstream history after that point is not open
# source and must never enter this repository.

set -euo pipefail

# Common: Add aligned_unique_ptr - last commit with every file under GPL-3.0.
GPL_BASE=14ab642ff3d5d489d388c8062530aa2f7c9a1372
# AudioStream: Rewrite and integrate expander - first CC-BY-NC-ND-only file.
FIRST_NONFREE=3fbdcdb4ff82ad572526cb240ab77675f0030718
# An SPDX header whose licence expression starts with a non-commercial CC
# licence. Dual "(GPL-3.0 OR CC-BY-NC-ND-4.0)" headers do not match.
NONFREE_HEADER='SPDX-License-Identifier:[[:space:]]*\(?[[:space:]]*CC-BY-NC'

rev=${1:-HEAD}
status=0

fail() {
  echo "license check: $*" >&2
  status=1
}

if git cat-file -e "${FIRST_NONFREE}^{commit}" 2>/dev/null &&
   git merge-base --is-ancestor "$FIRST_NONFREE" "$rev"; then
  fail "$rev contains upstream DuckStation history after the last GPL commit"
fi

if ! git merge-base --is-ancestor "$GPL_BASE" "$rev" 2>/dev/null; then
  fail "$rev is not based on the GPL base $GPL_BASE"
fi

if ! git show "$rev:LICENSE" 2>/dev/null | grep -q 'GNU GENERAL PUBLIC LICENSE'; then
  fail "$rev: LICENSE is not the GPL"
fi

# Check every commit since the base, so a non-GPL file that was added and later
# removed is still caught. If the history check already failed, only check the
# tip: scanning thousands of upstream commits would add nothing.
if [ "$status" -eq 0 ]; then
  commits=$(git rev-list "$GPL_BASE..$rev")
else
  commits=$(git rev-parse "$rev^{commit}")
fi

for commit in $commits; do
  hits=$(git grep -lE "$NONFREE_HEADER" "$commit" || true)
  if [ -n "$hits" ]; then
    fail "$(git log -1 --format='%h %s' "$commit") contains non-GPL files:"
    echo "$hits" | sed 's/^/  /' >&2
  fi
done

if [ "$status" -ne 0 ]; then
  echo "license check: see CONTRIBUTING.md" >&2
fi
exit "$status"