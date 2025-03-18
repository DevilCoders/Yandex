#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

run mkdir -p w-generated/all

# generate proton configs
run cp custom_generators/proton/* w-generated/all/

# dump main group owners (specially for osol and zhmurov)
run ./utils/common/dump_owners.py

# dump maximum unworking counts for base groups
#run ./utils/common/dump_unworking_limits.py -g SAS_WEB_BASE,MAN_WEB_BASE,VLA_WEB_BASE,SAS_IMGS_BASE,MAN_IMGS_BASE,VLA_IMGS_BASE,SAS_VIDEO_BASE,MAN_VIDEO_BASE,VLA_VIDEO_BASE -o w-generated/group.unworking.counts

# dump groups power
run ./utils/common/dump_power.py
