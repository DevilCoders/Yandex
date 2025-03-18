#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Clear old intlookups intlookups"
    run utils/common/reset_intlookups.py -g MAN_WEB_FUSION_3DAY_BASE_COMTRBACKUP,MAN_WEB_FUSION_10DAY_BASE_COMTRBACKUP -d
    echo "Clearing custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g MAN_WEB_FUSION_3DAY_BASE_COMTRBACKUP,MAN_WEB_FUSION_10DAY_BASE_COMTRBACKUP
    run ./utils/common/update_igroups.py -a emptygroup -g MAN_WEB_FUSION_10DAY_BASE_COMTRBACKUP
    echo 'Update hardware data'
    run utils/pregen/update_hosts.py -a update -g MAN_WEB_FUSION_3DAY_BASE_COMTRBACKUP --update-older-than 5 -p --ignore-group-constraints --ignore-detect-fail -y
}

function allocate_hosts() {
    echo "Nothing to allocate"
}

function recluster() {
    echo "Clearing custom instance power"
    run ./optimizers/sas/main.py -g MAN_WEB_FUSION_3DAY_BASE_COMTRBACKUP -t optimizers/sas/configs/man.fusion.tur.yaml -o 1 -e max
    echo "Moving 10day to slave group and unflat it"
    run ./utils/common/move_intlookup_to_slave.py -i intlookup-man-web-fusion-10day-comtrbackup.py -g MAN_WEB_FUSION_10DAY_BASE_COMTRBACKUP
    run ./utils/common/flat_intlookup.py -a unflat -i intlookup-man-web-fusion-10day-comtrbackup.py -o 20
    run ./utils/postgen/add_ints_everywhere.py -i intlookup-man-web-fusion-10day-comtrbackup.py -n 6 -N 4 -I 1 -a
}

select_action cleanup allocate_hosts recluster
