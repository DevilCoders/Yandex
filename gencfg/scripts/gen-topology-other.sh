#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

# dump main group owners (specially for osol and zhmurov)
run ./utils/common/dump_owners.py

# dump groups power
run ./utils/common/dump_power.py

# dump all golovan tags
run ./utils/common/dump_golovan_tags.py -o w-generated/golovan.tags
