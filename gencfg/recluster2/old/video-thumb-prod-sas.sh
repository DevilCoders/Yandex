#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

source `dirname "${BASH_SOURCE[0]}"`/utils.sh

function cleanup() {
    echo "Reset imtub intlookups"
    run ./utils/common/reset_intlookups.py -g SAS_VIDEO_THUMB
    echo "Update hardware data in SAS_VIDEO_THUMB"
    run utils/pregen/update_hosts.py -a update -g SAS_VIDEO_THUMB  --ignore-unknown-lastupdate --update-older-than 5 -p --ignore-detect-fail -y --ignore-group-constraints
    echo "Cycle unworking hosts"
    run recluster2/prepare.sh cycle_hosts SAS_VIDEO_THUMB
}

function allocate_hosts() {
    echo "Nothing to do"
}

function recluster() {
    echo "Recluster imtub intlookup"
    run ./utils/pregen/generate_custom_intlookup.py -t SAS_VIDEO_THUMB -b 1 -i 0 -s VtubTier0 -f noskip_score,skip_cluster_score,three_replicas -o intlookup-sas-video-thumb.py
    run ./utils/postgen/shift_intlookup.py -i intlookup-sas-video-thumb.py -o intlookup-sas-video-thumb-nidx.py -t SAS_VIDEO_THUMB_NIDX
}

select_action cleanup allocate_hosts recluster
