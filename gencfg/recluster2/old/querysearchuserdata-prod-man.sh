#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Empty MSK_QUERYSEARCH_USERDATA+slaves"
    run ./utils/common/reset_intlookups.py -g MAN_QUERYSEARCH_USERDATA
    run ./utils/common/update_igroups.py -a emptygroup -g MAN_QUERYSEARCH_USERDATA_PIP
}

function allocate_hosts() {
    echo "Allocating hosts for PIP"
    run ./utils/pregen/find_priemka_in_production_hosts_v2.py -r 2 -t QuerysearchUserdataTier0 -a QuerysearchUserdataTier0:1 -g MAN_QUERYSEARCH_USERDATA -s MAN_QUERYSEARCH_USERDATA_PIP --slot-size 12 --master-slot-size 12 --slots-per-host 2 --clear-current-hosts
}

function recluster() {
    echo "Recluster PIP"
    run ./utils/pregen/generate_trivial_intlookup.py -g MAN_QUERYSEARCH_USERDATA_PIP -b 1 -s QuerysearchUserdataTier0 -o intlookup-man-querysearch-userdata-pip.py
    echo "Recluster production"
    run ./utils/pregen/generate_custom_intlookup.py -t MAN_QUERYSEARCH_USERDATA -b 1 -i 0 -s QuerysearchUserdataTier0 -f noskip_score -o intlookup-man-querysearch-userdata.py
    echo "Check constraints"
    run ./utils/check/check_disk_size.py -i intlookup-man-querysearch-userdata-pip.py,intlookup-man-querysearch-userdata.py
    run ./utils/common/show_replicas_count.py -i intlookup-man-querysearch-userdata.py -m 3
    run ./utils/common/show_replicas_count.py -i intlookup-man-querysearch-userdata-pip.py -m 2
}

select_action cleanup allocate_hosts recluster
