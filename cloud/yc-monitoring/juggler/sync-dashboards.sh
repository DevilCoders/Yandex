#!/bin/bash
set -e
cd "$(dirname "$0")"

shopt -s globstar # For supporting ** in the next command

./juggler-dashboard.py upload --file dashboards/**/*.yaml.j2 "$@"
