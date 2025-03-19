#!/usr/bin/env bash

# Exit if any of the intermediate steps fail
set -e

COMMAND="ya vault oauth --skip-warnings"

set +e
for ((i=0;i<10;i++)); do
  output=$( ${COMMAND} 2>/dev/null)
  if [[ $? -eq 0 ]]; then
    break
  fi
  sleep 0.1
done
if [[ $? -ne 0 ]]; then
  echo "$COMMAND call return non 0 exit code, exit with 1" >&2
  exit 1
fi
set -e

jq --arg oauth "${output}" '{oauth: $oauth}'
