#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Calculate new weights"
    run ./tools/analyzer/analyzer.py -f update.sqlite -s optimizers/sas/configs/sas.refresh.yaml -t 200 -u instance_cpu_usage,host_cpu_usage
    run ./utils/common/update_sas_optimizer_config.py -c optimizers/sas/configs/sas.refresh.yaml -d update.sqlite -y
    echo "Clear old intlookups intlookups"
    run utils/common/reset_intlookups.py -g SAS_WEB_REFRESH_3DAY_BASE,SAS_WEB_REFRESH_10DAY_BASE -d
    echo "Clearing custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g SAS_WEB_REFRESH_3DAY_BASE,SAS_WEB_REFRESH_10DAY_BASE
    run ./utils/common/update_igroups.py -a emptygroup -g SAS_WEB_REFRESH_10DAY_BASE
    echo 'Update hardware data'
    run utils/pregen/update_hosts.py -a update -g SAS_WEB_REFRESH_3DAY_BASE --update-older-than 5 -p --ignore-group-constraints --ignore-detect-fail -y
}

function allocate_hosts() {
    echo "Nothing to allocate"
}

function recluster() {
    echo "Clearing custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g SAS_WEB_REFRESH_3DAY_BASE
    run ./optimizers/sas/main.py -g SAS_WEB_REFRESH_3DAY_BASE -t optimizers/sas/configs/sas.refresh.yaml -o 1 -e max
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/sas.refresh.yaml
    echo "Moving 10day to slave group and unflat it"
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_REFRESH_10DAY_BASE -g SAS_WEB_REFRESH_10DAY_BASE
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_REFRESH_10DAY_BASE -o 16
    run ./utils/postgen/add_ints_everywhere.py -i SAS_WEB_REFRESH_10DAY_BASE -n 6 -N 4 -I 1 -a
    echo "Moving tur 3day to slave group"
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_REFRESH_3DAY_BASE_COMTRBACKUP -g SAS_WEB_REFRESH_3DAY_BASE_COMTRBACKUP
    echo "Moving tur 10day to slave group and unflat it"
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP -g SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP -o 20 --cut-extra
    run ./utils/postgen/add_ints_everywhere.py -i SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP -n 6 -N 4 -I 1 -a
    echo "Checking constraints"
    run ./utils/check/check_instances_per_host.py -i SAS_WEB_REFRESH_3DAY_BASE,SAS_WEB_REFRESH_10DAY_BASE -m 2
}

select_action cleanup allocate_hosts recluster
