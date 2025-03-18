#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    run utils/common/reset_intlookups.py -g ALL_TSNET_BASE
}

function allocate_hosts() {
    echo "Nothing to do"
}

function recluster() {
    echo "Generating intlookups"
    run ./utils/pregen/generate_trivial_intlookup.py -g ALL_TSNET_BASE -b 1 -s PlatinumTier0+PlatinumTier0+RRGTier0+EngTier0+TurTier0 --uniform-by-host -m 1
}
