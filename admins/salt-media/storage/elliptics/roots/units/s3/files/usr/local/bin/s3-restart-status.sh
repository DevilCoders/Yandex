#!/usr/bin/env bash

set -o errexit
set -o nounset

status_file="/var/tmp/s3-restart/status"

if [ -r "${status_file}" ]; then
  cat "${status_file}"
else
  echo "0;File '${status_file}' not found"
fi
