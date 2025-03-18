#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

MYTIERS="VideoTier0:1"

function cleanup() {
    echo 'Empty ALL_VIDEO_BASE_R1+slaves'
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_VIDEO_BASE_R1_ONDISK
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_VIDEO_BASE_R1_BACKUP
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_VIDEO_BASE_R1
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_VIDEO_R1_ALIAS -c MSK_RESERVED
}

function allocate_hosts() {
    echo "Adding R1 hosts"
    run ./utils/pregen/find_r1_priemka_hosts_v3.py -g ALL_VIDEO_BASE_R1 -f MSK_RESERVED -t VideoTier0:1 -p 0 -s 62 -e 4 -m 10 -l "lambda x: x.ssd == 0 and x.raid == 'raid10' and x.n_disks == 4"
}

function recluster() {
    echo "Generating intlookups"
    run ./tools/recluster/main.py -a generate_intlookups -g ALL_VIDEO_BASE_R1_ONDISK
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_VIDEO_BASE_R1 -o 36
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_VIDEO_BASE_R1_BACKUP -o 36
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_VIDEO_BASE_R1_ONDISK -o 36
    echo "Adjusting intlookups and adding ints"
    run ./utils/postgen/adjust_r1.py -i ALL_VIDEO_BASE_R1 -b ALL_VIDEO_BASE_R1_BACKUP -a
    run ./utils/postgen/add_ints_everywhere.py -i ALL_VIDEO_BASE_R1 -n 4 -N 4 -a
    echo "Check constraints"
    run ./utils/check/check_disk_size.py -i ALL_VIDEO_BASE_R1,ALL_VIDEO_BASE_R1_BACKUP
    run ./utils/common/show_replicas_count.py -i ALL_VIDEO_BASE_R1,ALL_VIDEO_BASE_R1_BACKUP -j -m 2
}

select_action cleanup allocate_hosts recluster
