#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

run ./custom_generators/intmetasearchv2/generate_configs.py -a checkintersect
