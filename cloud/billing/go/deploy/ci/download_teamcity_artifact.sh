#!/bin/bash
set -e

# Download teamcity artifact with authorization by token
# Args:
#   $1 - source url
#   $2 - output file
# Environment variables:
#   TC_AUTH_TOKEN - token from secret for teamcity auth

curl -L -H "Authorization: Bearer $TC_AUTH_TOKEN" "$1" -o "$2"
