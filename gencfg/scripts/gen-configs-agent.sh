#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

WEB_GENERATED_DIR="w-generated/all"

run custom_generators/agents/agents.py --target-dir ${WEB_GENERATED_DIR}
