#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

#MYTIERS="OxygenExpPlatinumTier0,OxygenExpPlatinumTurTier0"
#MYTIERS_SLOTS="OxygenExpPlatinumTier0:1,OxygenExpPlatinumTurTier0:1"
MYTIERS="OxygenExpPlatinumTier0"
MYTIERS_COMMON="OxygenExpRusMaps,OxygenExpTurMaps,OxygenExpDivTier0,OxygenExpDivTrTier0"

function cleanup() {
    run ./tools/recluster/main.py -a recluster -g MAN_OXYGEN_IO_BASE -c cleanup
    run ./tools/recluster/main.py -a recluster -g MAN_OXYGEN_IO_OXYGEN_BACKUP_BASE -c cleanup
}

function allocate_hosts() {
    /bin/true
}

function recluster() {
    echo "Recluster MAN_OXYGEN_IO_BASE"
    run ./tools/recluster/main.py -a recluster -g MAN_OXYGEN_IO_BASE -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_OXYGEN_IO_INT -c generate_intlookups

    echo "Recluster MAN_OXYGEN_IO_OXYGEN_BACKUP_BASE"
    run ./tools/recluster/main.py -a recluster -g MAN_OXYGEN_IO_OXYGEN_BACKUP_BASE -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_OXYGEN_IO_OXYGEN_BACKUP_INT -c generate_intlookups

    echo "Check constraints"
    run ./utils/common/show_replicas_count.py -i MAN_OXYGEN_IO_BASE.OxygenExpPlatinumTier0,MAN_OXYGEN_IO_BASE.OxygenExpDivTier0,MAN_OXYGEN_IO_BASE.OxygenExpDivTrTier0 -m 2
    run ./utils/common/show_replicas_count.py -i MAN_OXYGEN_IO_OXYGEN_BACKUP_BASE.OxygenExpPlatinumTier0,MAN_OXYGEN_IO_OXYGEN_BACKUP_BASE.OxygenExpDivTier0,MAN_OXYGEN_IO_OXYGEN_BACKUP_BASE.OxygenExpDivTrTier0 -m 2
}

select_action cleanup allocate_hosts recluster
