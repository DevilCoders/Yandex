#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

MYTIERS="PlatinumTier0:1,WebTier0:1"
MYTIERSSHORT="PlatinumTier0,WebTier0"

function cleanup() {
    echo 'Remove MSK_WEB_BASE_PRIEMKA+slaves intlookups...'
    run utils/common/reset_intlookups.py -g MSK_WEB_BASE_PRIEMKA,MSK_WEB_BASE_PRIEMKA_BACKUP,MSK_WEB_BASE_PRIEMKA_ONDISK -e
    echo 'Update hardware data in MSK_WEB_BASE_PRIEMKA'
    run utils/pregen/update_hosts.py -a update -g MSK_WEB_BASE_PRIEMKA --update-older-than 5 -p --ignore-group-constraints --ignore-detect-fail -y
    echo "Cycle unworking hosts"
    run recluster2/prepare.sh cycle_hosts MSK_WEB_BASE_PRIEMKA
    echo 'Empty MSK_WEB_BASE_PRIEMKA+slaves...'
    run utils/common/update_igroups.py -a emptygroup -g MSK_WEB_BASE_PRIEMKA -c MSK_RESERVED --remove-from-master
    run utils/common/update_igroups.py -a emptygroup -g MSK_WEB_BASE_PRIEMKA_BACKUP -c MSK_RESERVED --remove-from-master
    run utils/common/update_igroups.py -a emptygroup -g MSK_WEB_BASE_PRIEMKA_ONDISK -c MSK_RESERVED --remove-from-master
}

function allocate_hosts() {
    echo "Adding priemka hosts"
    run ./utils/pregen/find_r1_priemka_hosts_v3.py -g MSK_WEB_BASE_PRIEMKA -f MSK_WEB_BASE -t ${MYTIERS} -p 0 -s 48 -e 4 -m 10 -l "lambda x: x.memory < 200 and x.memory >= 96 and x.raid == 'raid10' and x.ssd == 0 and x.disk >= 460 and x.location == 'msk' and (not x.model.startswith('AMD'))"
}

function recluster() {
    echo "Generating intlookups"
    run ./utils/common/generate_ondisk_backup.py -g MSK_WEB_BASE_PRIEMKA_ONDISK -r 1 -m 7 -i MSK_WEB_BASE_PRIEMKA_ONDISK -t ${MYTIERSSHORT} -o MSK_WEB_BASE_PRIEMKA,MSK_WEB_BASE_PRIEMKA_BACKUP
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_PRIEMKA -o 18
    run ./utils/common/flat_intlookup.py -a unflat -i MSK_WEB_BASE_PRIEMKA_BACKUP -o 18
    echo "Adjusting and adding ints"
    run ./utils/postgen/adjust_r1.py -i intlookups/MSK_WEB_BASE_PRIEMKA -b intlookups/MSK_WEB_BASE_PRIEMKA_BACKUP -a

    run ./utils/common/update_intlookup.py -a split -s MSK_WEB_BASE_PRIEMKA -y
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i MSK_WEB_BASE_PRIEMKA.WebTier0
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i MSK_WEB_BASE_PRIEMKA.WebTier0 -n 10
    run ./utils/postgen/add_ints_everywhere.py -i MSK_WEB_BASE_PRIEMKA.WebTier0 -n 2 -N 2 -a

    run ./utils/common/update_intlookup.py -a split -s MSK_WEB_BASE_PRIEMKA_BACKUP -y
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i MSK_WEB_BASE_PRIEMKA_BACKUP.WebTier0

    run ./utils/common/update_intlookup.py -a split -s MSK_WEB_BASE_PRIEMKA_ONDISK -y
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i MSK_WEB_BASE_PRIEMKA_ONDISK.WebTier0

    run ./utils/postgen/add_ints2.py -a alloc_int -i MSK_WEB_BASE_PRIEMKA -n 2 -f "lambda x: x.host.memory > 100"
    echo "Check constraints"
    run ./utils/check/check_disk_size.py -i MSK_WEB_BASE_PRIEMKA,MSK_WEB_BASE_PRIEMKA_BACKUP
    run ./utils/common/show_replicas_count.py -i MSK_WEB_BASE_PRIEMKA.PlatinumTier0,MSK_WEB_BASE_PRIEMKA.WebTier0,MSK_WEB_BASE_PRIEMKA_BACKUP.PlatinumTier0,MSK_WEB_BASE_PRIEMKA_BACKUP.WebTier0,MSK_WEB_BASE_PRIEMKA_ONDISK.PlatinumTier0,MSK_WEB_BASE_PRIEMKA_ONDISK.WebTier0 -j -m 3
}

select_action cleanup allocate_hosts recluster
