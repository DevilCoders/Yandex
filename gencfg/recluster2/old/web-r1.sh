#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

MYTIERS="RRGTier0:1,PlatinumTier0:1,EngTier0:1,TurTier0:1,PlatinumTurTier0:1"
MYTIERSSHORT="RRGTier0,PlatinumTier0,EngTier0,TurTier0,PlatinumTurTier0"

function cleanup() {
    echo "Cleanup other slave groups"
    run ./tools/recluster/main.py -a recluster -g MSK_WEB_BASE_R1_ONDISK -c cleanup
    run ./tools/recluster/main.py -a recluster -g MSK_WEB_BASE_R1_BACKUP -c cleanup
    run ./tools/recluster/main.py -a recluster -g MSK_WEB_BASE_R1 -c cleanup

    echo 'Update hardware data in MSK_WEB_BASE_R1'
    run utils/pregen/update_hosts.py -a update -g ALL_WEB_R1_ALIAS --update-older-than 5 -p --ignore-group-constraints --ignore-detect-fail -y
    echo "Cycle unworking hosts"
    run recluster2/prepare.sh cycle_hosts ALL_WEB_R1_ALIAS
    echo 'Empty ALL_WEB_R1_ALIAS'
    run utils/common/update_igroups.py -a emptygroup -g ALL_WEB_R1_ALIAS -c MSK_RESERVED
}

function allocate_hosts() {
    echo "Adding R1 hosts"
    run ./utils/pregen/find_r1_priemka_hosts_v3.py -g MSK_WEB_BASE_R1 -f MSK_RESERVED -t PlatinumTier0:1,WebTier0:1 -s 42 -e 3 -m 10 -l "lambda x: x.memory >= 48 and x.raid == 'raid10' and x.ssd == 0 and x.memory < 200 and x.disk > 500" --prefer-amd
}

function recluster() {
    echo "Generating intlookups"
    run ./utils/common/generate_ondisk_backup.py -g MSK_WEB_BASE_R1_ONDISK -r 1 -m 9 -i MSK_WEB_BASE_R1_ONDISK -t PlatinumTier0,WebTier0 -o MSK_WEB_BASE_R1,MSK_WEB_BASE_R1_BACKUP
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_R1 -o 18
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_R1_BACKUP -o 18
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_R1_ONDISK -o 18

    run ./utils/common/update_intlookup.py -a split -s MSK_WEB_BASE_R1 -y
    run ./utils/common/update_intlookup.py -a split -s MSK_WEB_BASE_R1_BACKUP -y
    run ./utils/common/update_intlookup.py -a split -s MSK_WEB_BASE_R1_ONDISK -y

    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_R1.WebTier0 --process-intl2 -o 16
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_R1_BACKUP.WebTier0 --process-intl2 -o 16
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_R1_ONDISK.WebTier0 --process-intl2 -o 16
    echo "Adjusting intlookups and adding ints"
    run ./utils/postgen/adjust_r1.py -i MSK_WEB_BASE_R1.PlatinumTier0 -b MSK_WEB_BASE_R1_BACKUP.PlatinumTier0 -a
    run ./utils/postgen/adjust_r1.py -i MSK_WEB_BASE_R1.WebTier0 -b MSK_WEB_BASE_R1_BACKUP.WebTier0 -a
    run ./utils/postgen/add_ints_everywhere.py -i MSK_WEB_BASE_R1.PlatinumTier0,MSK_WEB_BASE_R1.WebTier0 -n 2 -N 2 -a
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i MSK_WEB_BASE_R1.WebTier0 -n 10
    echo "Check constraints"
    run ./utils/check/check_disk_size.py -i MSK_WEB_BASE_R1.WebTier0,MSK_WEB_BASE_R1.PlatinumTier0,MSK_WEB_BASE_R1_BACKUP.WebTier0,MSK_WEB_BASE_R1_BACKUP.PlatinumTier0,MSK_WEB_BASE_R1_ONDISK.WebTier0,MSK_WEB_BASE_R1_ONDISK.PlatinumTier0
    run ./utils/common/show_replicas_count.py -i MSK_WEB_BASE_R1.WebTier0,MSK_WEB_BASE_R1_BACKUP.WebTier0,MSK_WEB_BASE_R1_ONDISK.WebTier0 -j -m 3
    run ./utils/common/show_replicas_count.py -i MSK_WEB_BASE_R1.PlatinumTier0,MSK_WEB_BASE_R1_BACKUP.PlatinumTier0,MSK_WEB_BASE_R1_ONDISK.PlatinumTier0 -j -m 3
}

select_action cleanup allocate_hosts recluster
