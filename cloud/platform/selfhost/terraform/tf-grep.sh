#!/usr/bin/env bash
set -eo pipefail

case "$1" in
"help"|"--help"|"-h")
  echo "Greps 'terraform plan'."
  echo 'Usage:'
  echo
  echo "  terraform plan | $0"
  exit 0
  ;;
esac

PLAN=$(cat)

grep -2 -E '[[]3[0-7]m[-+~]' <<< "$PLAN"
echo
echo ........................................................................
echo
tail <<< "$PLAN"
