#!/usr/bin/env bash

# Exit if any of the intermediate steps fail
set -e

# Extract arguments from the input into shell variables.
# jq will ensure that the values are properly quoted
# and escaped for consumption by the shell.
eval "$(jq -r '@sh "SECRET_ID=\(.secret_id) KEY_ID=\(.key_id)"')"

if [[ $SECRET_ID == "null" ]]; then
  echo "there is no secret_id defined!"
  exit 1
fi

COMMAND_META="ya vault get version ${SECRET_ID} -j --skip-warnings"
COMMAND_DATA="ya vault get version ${SECRET_ID} -o ${KEY_ID}"

set +e

for i in {1..10}; do
  output_meta=$(${COMMAND_META} 2>/dev/null)
  if [[ $? -eq 0 ]]; then
    break
  fi
  sleep 0.1
done
if [[ $? -ne 0 ]]; then
  echo "$COMMAND_META call return non 0 exit code, exit with 1" >&2
  exit 1
fi

for i in {1..10}; do
  output_data=$(${COMMAND_DATA} 2>/dev/null)
  if [[ $? -eq 0 ]]; then
    break
  fi
  sleep 0.1
done
if [[ $? -ne 0 ]]; then
  echo "$COMMAND_DATA call return non 0 exit code, exit with 1" >&2
  exit 1
fi

set -e

echo "${output_meta}" | jq --arg value "$output_data" --arg key_id "$KEY_ID" '{secret_name: .secret_name, key: $key_id, value: $value }'
