#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 mentalfoundry
# SPDX-License-Identifier: GPL-3.0
# Writes the notes of a rolling release, in Markdown, to standard output.
#
# Usage: release-notes.sh <previous release commit, or empty> <release commit> <owner/repository>
#
# The list of changes comes from the "Changelog:" trailers of the commits after the previous release. See
# CONTRIBUTING.md.

set -euo pipefail

prev=${1:-}
cur=$2
repo=$3

echo "Windows x64 builds of ${cur}. The release has the PokketSX application and the PokketSX libretro core. The libretro core is experimental."
echo
echo "## Changes"
echo

# Without a previous release that this history contains, list the trailers of all commits.
if [ -n "$prev" ] && git merge-base --is-ancestor "$prev" "$cur" 2>/dev/null; then
  range="${prev}..${cur}"
else
  prev=""
  range="$cur"
fi

changes=$(git log --reverse --format='%(trailers:key=Changelog,valueonly,unfold,separator=%x0A)' "$range" | sed '/^[[:space:]]*$/d')
if [ -n "$changes" ]; then
  printf '%s\n' "$changes" | sed 's/^/- /'
else
  echo "This release has only changes for developers."
fi

if [ -n "$prev" ]; then
  echo
  echo "All commits since the previous release: https://github.com/${repo}/compare/${prev}...${cur}"
fi
