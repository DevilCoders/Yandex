#!/usr/bin/env bash
set -eo pipefail

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Copy certificate from one YAV secret to another"
  echo 
  echo "Usage:"
  echo
  echo "  $0 sec-oldsecret sec-newsecret"
  echo
  echo 'Secret values names will be changed to "crt" and "key".'
  exit 0
  ;;
esac

SECRET=$(yav get version "$1" --json | \
         jq '.value' | \
         sed -E 's/[A-F0-9]+_private_key/key/; s/[A-F0-9]+_certificate/crt/;' )

COMMENT=$(jq -r .crt <<< "$SECRET" | \
          openssl x509 -serial -dates -ext subjectAltName -nocert)

yav create version "$2" -v "$SECRET" -c "$COMMENT"
