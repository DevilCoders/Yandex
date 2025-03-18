#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/scripts/run.sh

rm -f wheels/*
rm -f utils/binary/*
rm -f core/*.so

rm -f optimizer
rm -rf venv

custom_generators/balancer_gencfg/uninstall.sh

# cleanup contrib packages
make clean -C contrib/cJSON/
