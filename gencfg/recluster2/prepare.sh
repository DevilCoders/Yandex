#!/usr/bin/env bash

# this script updates all hardware and clean up unworking machines
# you should run it after first recluster stage if you want check hw in production groups

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh
source `dirname "${BASH_SOURCE[0]}"`/../recluster2/utils.sh

function prepare_safe() {
    rename_by_invnum
    sync_with_bot
#    udpate_tiers_sizes
    update_unworking
    update_hw
}

function prepare_web_weights() {
    update_sas_optimizer_weights optimizers/sas/configs/msk.web.yaml optimizers/sas/configs/sas.web.yaml optimizers/sas/configs/man.web.yaml
}

$@
