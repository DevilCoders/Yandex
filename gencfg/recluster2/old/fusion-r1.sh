#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Clear old intlookups intlookups"
    run utils/common/reset_intlookups.py -g MSK_FUSION_BASE_R1 -d
    echo 'Update hardware data'
    run utils/pregen/update_hosts.py -a update -g MSK_FUSION_BASE_R1 --update-older-than 5 -p --ignore-group-constraints --ignore-detect-fail -y
}

function allocate_hosts() {
    echo "Nothing to allocate"
}

function recluster() {
    echo "Creating intlookups"
    run ./utils/common/create_simple_intlookup.py -t Fusion3dayTier0 -g MSK_FUSION_BASE_R1 -i intlookup-msk-fusion-3day-p.py -e
    run ./utils/common/create_simple_intlookup.py -t Fusion10dayTier0 -g MSK_FUSION_BASE_R1 -i intlookup-msk-fusion-10day-p.py -e
    run ./utils/common/add_extra_replicas.py -a addextra -r 1 -i intlookup-msk-fusion-3day-p.py -f "lambda x: x.host.dc in ['fol', 'ugrA']"
    run ./utils/common/add_extra_replicas.py -a addextra -r 1 -i intlookup-msk-fusion-10day-p.py -f "lambda x: x.host.dc in ['fol', 'ugrA']"
}

select_action cleanup allocate_hosts recluster
