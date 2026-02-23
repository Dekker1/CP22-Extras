#!/usr/bin/env bash
set -euo pipefail

# Update all software/* git subtrees to feat/blackbox.
# Usage:
#   ./update_softare.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "error: must be run inside a git repository" >&2
  exit 1
fi

TARGET_BRANCH="feat/blackbox"

declare -A SUBTREE_REMOTE=(
  [chuffed]="https://github.com/Dekker1/chuffed.git"
  [gecode]="https://github.com/Dekker1/gecode.git"
  [minizinc]="git@gitlab.com:minizinc/minizinc.git"
)

updated_any=0
for prefix_path in software/*; do
  [ -d "$prefix_path" ] || continue

  name="$(basename "$prefix_path")"
  remote="${SUBTREE_REMOTE[$name]-}"

  if [ -z "${remote}" ]; then
    echo "skip: no remote configured for $prefix_path"
    continue
  fi

  echo "updating $prefix_path from $remote ($TARGET_BRANCH)"
  git subtree pull --prefix "$prefix_path" "$remote" "$TARGET_BRANCH" --squash
  updated_any=1
done

if [ "$updated_any" -eq 0 ]; then
  echo "warning: no subtree updates were run"
fi
