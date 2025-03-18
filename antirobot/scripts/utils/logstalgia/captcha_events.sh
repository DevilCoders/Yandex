#!/usr/bin/env bash
set -eo pipefail

BASE_DIR=`dirname $0`

case "$1" in
-h|--help) $BASE_DIR/_captcha_events.py "$@"
;;
*) $BASE_DIR/_captcha_events.py "$@" | tr \\t '|' | tr -d ' '
;;
esac
