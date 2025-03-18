#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh
run make clean_service
run time make build/service/all -j4
