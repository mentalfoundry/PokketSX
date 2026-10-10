#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 mentalfoundry
# SPDX-License-Identifier: GPL-3.0
# Fails if a revision (default HEAD), or any commit between the GPL base and it,
# contains code that is not available under the GPL.
#
# PokketSX is a hard fork of the last DuckStation commit in which every file
# was available under GPL-3.0. Upstream history after that point is not open
# source and must never enter this repository.
#
# Every SPDX-License-Identifier must allow use under GPL-3.0: it must either
# offer a GPL-compatible licence as an alternative, such as
# "(GPL-3.0 OR CC-BY-NC-ND-4.0)", or consist only of GPL-compatible licences.
# Any other licence fails, whatever its name.

set -euo pipefail

# Common: Add aligned_unique_ptr - last commit with every file under GPL-3.0.
GPL_BASE=14ab642ff3d5d489d388c8062530aa2f7c9a1372
# AudioStream: Rewrite and integrate expander - first CC-BY-NC-ND-only file.
FIRST_NONFREE=3fbdcdb4ff82ad572526cb240ab77675f0030718

TAB=$'\t'
rev=${1:-HEAD}
status=0

fail() {
  echo "license check: $*" >&2
  status=1
}

# True if an SPDX licence identifier may be combined with GPL-3.0 code.
gpl_compatible_licence() {
  case "$1" in
    GPL-3.0 | GPL-3.0+ | GPL-3.0-only | GPL-3.0-or-later | GPL-2.0+ | GPL-2.0-or-later | \
    LGPL-2.1 | LGPL-2.1+ | LGPL-2.1-only | LGPL-2.1-or-later | \
    LGPL-3.0 | LGPL-3.0+ | LGPL-3.0-only | LGPL-3.0-or-later | \
    MIT | BSD-2-Clause | BSD-3-Clause | 0BSD | ISC | Zlib | BSL-1.0 | \
    Apache-2.0 | MPL-2.0 | CC0-1.0 | Unlicense | WTFPL)
      return 0
      ;;
  esac
  return 1
}

# True if an SPDX licence expression allows use under GPL-3.0. Each licence
# becomes 1 or 0 and the expression is evaluated as arithmetic. An exception
# ("X WITH Y") only adds permissions, so it is judged as X. A malformed
# expression, such as two licences with no operator between them, fails.
gpl_compatible_expression() {
  local tokens tok arith="" want_operand=1
  tokens=$(printf '%s' "$1" | sed -E 's/[[:space:]]+WITH[[:space:]]+[^[:space:]()]+//g; s/[()]/ & /g')
  set -f
  for tok in $tokens; do
    case "$tok" in
      "(")
        [ "$want_operand" -eq 1 ] || { set +f; return 1; }
        arith+="("
        ;;
      ")")
        [ "$want_operand" -eq 0 ] || { set +f; return 1; }
        arith+=")"
        ;;
      OR | AND)
        [ "$want_operand" -eq 0 ] || { set +f; return 1; }
        if [ "$tok" = OR ]; then arith+=" || "; else arith+=" && "; fi
        want_operand=1
        ;;
      *)
        [ "$want_operand" -eq 1 ] || { set +f; return 1; }
        if gpl_compatible_licence "$tok"; then arith+=1; else arith+=0; fi
        want_operand=0
        ;;
    esac
  done
  set +f
  [ -n "$arith" ] && [ "$want_operand" -eq 0 ] && (( $arith )) 2>/dev/null
}

# "path<TAB>licence expression" for every SPDX licence line in a commit.
spdx_entries() {
  { git grep -I -o -E 'SPDX-License-Identifier:.*' "$1" -- ':!scripts/check-license.sh' || true; } |
    tr -d '\r' |
    sed -E "s/^$1://; s/:SPDX-License-Identifier:[[:space:]]*/${TAB}/; s/[[:space:]]*(\*\/|-->)?[[:space:]]*\$//"
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
  bad=""
  while IFS= read -r expr; do
    gpl_compatible_expression "$expr" || bad+="$expr"$'\n'
  done < <(spdx_entries "$commit" | cut -f2 | sort -u)

  if [ -n "$bad" ]; then
    fail "$(git log -1 --format='%h %s' "$commit") has licences that are not GPL-compatible:"
    spdx_entries "$commit" | while IFS="$TAB" read -r path expr; do
      if printf '%s' "$bad" | grep -qxF -- "$expr"; then
        echo "  $path: $expr" >&2
      fi
    done
  fi
done

if [ "$status" -ne 0 ]; then
  echo "license check: see CONTRIBUTING.md" >&2
fi
exit "$status"