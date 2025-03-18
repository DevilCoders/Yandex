#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

#MYTIERS="OxygenExpPlatinumTier0,OxygenExpPlatinumTurTier0"
#MYTIERS_SLOTS="OxygenExpPlatinumTier0:1,OxygenExpPlatinumTurTier0:1"
MYTIERS="OxygenExpPlatinumTier0"

function cleanup() {
    run ./utils/common/reset_intlookups.py -g MAN_OXYGEN_CALLISTO_BASE
}

function allocate_hosts() {
    /bin/true
}

function recluster() {
    echo "Recluster basesearchers"
    run ./utils/common/import_oxygen_shards.py -p ./robotdata/oxygencfg/shard-callisto.map -i MAN_OXYGEN_CALLISTO_BASE -s 36 -n 6 -g MAN_OXYGEN_CALLISTO_BASE -t ${MYTIERS}

    echo "Add ints"
    run ./utils/postgen/add_ints_everywhere.py -i MAN_OXYGEN_CALLISTO_BASE -n 3 -N 3 -d -A -t MAN_OXYGEN_CALLISTO_INT

    echo "Check constraints"
    run ./utils/common/show_replicas_count.py -i MAN_OXYGEN_CALLISTO_BASE -m 2
}

select_action cleanup allocate_hosts recluster
