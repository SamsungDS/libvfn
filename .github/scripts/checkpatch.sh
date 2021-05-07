#!/bin/bash

set -euo pipefail

pr="${GITHUB_REF#"refs/pull/"}"
prnum="${pr%"/merge"}"
prurl="${GITHUB_API_URL}/repos/${GITHUB_REPOSITORY}/pulls/${prnum}/commits"

commits=( $(curl -H "Authorization: Bearer ${GITHUB_TOKEN}" -X GET -s "$prurl" | jq '.[].sha' -r) )

for commit in ${commits[@]}; do
  echo "::group::check ${commit}"
  if ! scripts/checkpatch.pl --git "$commit" | .github/scripts/checkpatch.awk; then
    FAILURE=
  fi
  echo "::endgroup::"
done

if [[ -v FAILURE ]]; then
  exit 1
fi
