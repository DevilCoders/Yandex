#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

MYTIERS="ImgTier0:1"

function cleanup() {
    echo 'Remove ALL_IMGS_BASE_PRIEMKA intlookups'
    run ./utils/common/reset_intlookups.py -g ALL_IMGS_BASE_PRIEMKA,ALL_IMGS_BASE_PRIEMKA_BACKUP,ALL_IMGS_CBIR_BASE_PRIEMKA,ALL_IMGS_QUICK_BASE_PRIEMKA,MSK_IMGS_CBIR_CBRD_PRIEMKA,MSK_IMGS_QUICK_THUMB_PRIEMKA -d
    echo 'Empty ALL_IMGS_BASE_PRIEMKA+slaves'
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_IMGS_BASE_PRIEMKA_BACKUP
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_IMGS_BASE_PRIEMKA
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_IMGS_CBIR_BASE_PRIEMKA
    run ./utils/common/update_igroups.py -a emptygroup -g ALL_IMGS_QUICK_BASE_PRIEMKA
    run ./utils/common/update_igroups.py -a emptygroup -g MSK_IMGS_CBIR_CBRD_PRIEMKA
    run ./utils/common/update_igroups.py -a emptygroup -g MSK_IMGS_QUICK_THUMB_PRIEMKA
    echo 'Update hardware data'
    run utils/pregen/update_hosts.py -a update -g ALL_IMGS_C1_ALIAS --update-older-than 5 -p --ignore-group-constraints --ignore-detect-fail -y
}

function allocate_hosts() {
    echo "Added priemka"
    run ./utils/pregen/find_r1_priemka_hosts_v3.py -g ALL_IMGS_BASE_C1 -f MSK_RESERVED,MSK_WEB_BASE  -t ImgTier0:1 -p 0 -s 40 -e 3 -m 10 -l "lambda x: x.ssd == 0 and x.raid == 'raid10' and x.n_disks == 4 and x.memory > 200"
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_IMGS_BASE_C1 -o 36
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_IMGS_BASE_C1_BACKUP -o 36
    run ./utils/postgen/adjust_r1.py -i ALL_IMGS_BASE_C1 -b ALL_IMGS_BASE_C1_BACKUP -a
    run ./utils/postgen/add_ints_everywhere.py -i ALL_IMGS_BASE_C1 -n 2 -N 2 -a
    run ./utils/check/check_disk_size.py -i ALL_IMGS_BASE_C1,ALL_IMGS_BASE_C1_BACKUP
    run ./utils/common/show_replicas_count.py -i ALL_IMGS_BASE_C1,ALL_IMGS_BASE_C1_BACKUP -j -m 2

    echo "Added R1"
    run ./utils/pregen/find_r1_priemka_hosts_v3.py -g ALL_IMGS_BASE_R1 -f MSK_RESERVED,MSK_WEB_BASE  -t ImgTier0:1 -p 0 -s 40 -e 3 -m 10 -l "lambda x: x.ssd == 0 and x.raid == 'raid10' and x.n_disks == 4 and x.memory > 200"
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_IMGS_BASE_R1 -o 36
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_IMGS_BASE_R1_BACKUP -o 36
    run ./utils/postgen/adjust_r1.py -i ALL_IMGS_BASE_R1 -b ALL_IMGS_BASE_R1_BACKUP -a
    run ./utils/postgen/add_ints_everywhere.py -i ALL_IMGS_BASE_R1 -n 4 -N 4 -a
    run ./utils/check/check_disk_size.py -i ALL_IMGS_BASE_R1,ALL_IMGS_BASE_R1_BACKUP
    run ./utils/common/show_replicas_count.py -i ALL_IMGS_BASE_R1,ALL_IMGS_BASE_R1_BACKUP -j -m 2

    echo "Added oldpip"
    run ./utils/pregen/find_r1_priemka_hosts_v3.py -g ALL_IMGS_BASE_OLDPIP -f MSK_RESERVED,MSK_WEB_BASE  -t ImgTier0:1 -p 0 -s 40 -e 3 -m 10 -l "lambda x: x.ssd == 0 and x.raid == 'raid10' and x.n_disks == 4 and x.memory > 200"
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_IMGS_BASE_OLDPIP -o 36
    run ./utils/common/flat_intlookup.py -a unflat -i ALL_IMGS_BASE_OLDPIP_BACKUP -o 36
    run ./utils/postgen/adjust_r1.py -i ALL_IMGS_BASE_OLDPIP -b ALL_IMGS_BASE_OLDPIP_BACKUP -a
    run ./utils/postgen/add_ints_everywhere.py -i ALL_IMGS_BASE_OLDPIP -n 2 -N 2 -a
    run ./utils/check/check_disk_size.py -i ALL_IMGS_BASE_OLDPIP,ALL_IMGS_BASE_OLDPIP_BACKUP
    run ./utils/common/show_replicas_count.py -i ALL_IMGS_BASE_OLDPIP,ALL_IMGS_BASE_OLDPIP_BACKUP -j -m 2

}

function recluster() {
    echo "Generating intlookups"
    run ./utils/pregen/generate_trivial_intlookup.py -g ALL_IMGS_CBIR_BASE_PRIEMKA -b 36 -s ImgCBIRTier0 -o ALL_IMGS_CBIR_BASE_PRIEMKA
    echo "Adjusting intlookups and adding ints"
    run ./utils/postgen/add_ints_everywhere.py -i ALL_IMGS_CBIR_BASE_PRIEMKA -n 2 -N 2 -a
    echo "Check constraints"
}

select_action cleanup allocate_hosts recluster
