#!/usr/bin/env bash

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Encrypts with KMS"
  echo 
  echo "Usage:"
  echo
  echo "  echo plaintext | $0 key-id --profile=testing"
  echo
  echo "Outputs JSON."
  exit 0
  ;;
esac

set -eo pipefail

{
  echo plaintext: $(base64 --wrap=0)
  echo key_id: $1
} | ycp kms symmetric-crypto encrypt -r- --format=json $2
