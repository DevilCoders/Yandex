#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

GENERATED_DIR="./generated/gemini"

mkdir -p ${GENERATED_DIR}

run ./custom_generators/gemini/gen_searchproxy.py ${GENERATED_DIR}/gemini-searchmap.json
