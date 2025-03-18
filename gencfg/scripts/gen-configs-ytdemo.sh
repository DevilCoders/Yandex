#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

GENERATED_DIR="./generated/ytdemo"

mkdir -p ${GENERATED_DIR}

run ./custom_generators/ytdemo/generate_config.py ${GENERATED_DIR}/ytdemo.cfg
