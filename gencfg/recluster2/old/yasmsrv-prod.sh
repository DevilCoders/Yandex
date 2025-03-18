#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Nothing to do"
}

function allocate_hosts() {
    echo "Nothing to do"
}

function recluster() {
    echo "Assigning hosts to slave groups"
    run ./utils/rare_utils/recluster_golovan_srv.py -c db/configs/yasmsrv/prod.txt -g ALL_YASM_SRV_NEW -r MSK_RESERVED,SAS_RESERVED,MAN_RESERVED

    # shift intlookups for current yasm srv
    run ./utils/postgen/shift_intlookup.py -i MSK_YASM_PRODUCTION_SRV -o MSK_YASM_PRODUCTION_DUMPER -t MSK_YASM_PRODUCTION_DUMPER
    run ./utils/postgen/shift_intlookup.py -i MSK_YASM_PRODUCTION_SRV -o MSK_YASM_PRODUCTION_HSRV -t MSK_YASM_PRODUCTION_HSRV
}
