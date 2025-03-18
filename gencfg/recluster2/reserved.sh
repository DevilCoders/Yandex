#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

# Function executed just before making new branch (next action after prepare_trunk is create new branch)
function prepare_trunk() {
    # Split reserved into two part: 100 machines remain in reserved untouched and will be in reserved in next branch
    # We have to keep some muchines, because we want to allocate something during recluster
    run ./utils/common/update_reserved_on_recluster.py -a prepare_trunk

    run ./utils/common/manipulate_sandbox_schedulers.py -a stop -t UPDATE_CONFIG_GENERATOR_DB
}

# Function executed just after creating new branch in branch
function prepare_branch() {
    # Move RESERVED to RESERVED_OLD to keep untouched during recluster
    run ./utils/common/update_reserved_on_recluster.py -a prepare_branch
}

# Function executed just before final merge in branch
function prepare_merge() {
    # find all still used hosts and move them to busy
    run ./utils/sky/show_hosts_with_raised_instances.py -a show_brief -s MSK_RESERVED,SAS_RESERVED,MAN_RESERVED,VLA_RESERVED --move-used-to-group ALL_UNWORKING_BUSY
    # Merge RESERVED and RESERVED_OLD
    run ./utils/common/update_reserved_on_recluster.py -a prepare_merge
}

$@
