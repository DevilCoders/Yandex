#!/usr/bin/env bash

# Exit if any of the intermediate steps fail
set -e

LOCKBOX_SECRET=$(yc lockbox payload get --format json --profile "$1" --id "$2")
LOCKBOX_SECRET_VERSION=$(echo "$LOCKBOX_SECRET" | jq " .version_id ")
LOCKBOX_SECRET_VALUE=$(echo "$LOCKBOX_SECRET" | jq " .entries | .[] | select(.key == \"$3\") | .text_value ")
echo "{
    \"id\": \"$2\",
    \"version\": $LOCKBOX_SECRET_VERSION,
    \"secret\": $LOCKBOX_SECRET_VALUE
}"
