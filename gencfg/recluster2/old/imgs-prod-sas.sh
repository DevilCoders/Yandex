#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Remove SAS_IMGS_BASE+slaves intlookups"
    run ./utils/common/reset_intlookups.py -g SAS_IMGS_BASE,SAS_IMGS_CBIR_BASE -s
    run ./tools/recluster/main.py -a recluster -c cleanup -g  SAS_IMGS_CBIR_INT
    run ./tools/recluster/main.py -a recluster -c cleanup -g SAS_IMGS_CBIR_BASE_HAMSTER
    run ./utils/common/update_igroups.py -a emptygroup -g SAS_IMGS_CBIR_BASE
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_RIM_NIDX -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_RIM -c cleanup
    echo "Cleanup custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g SAS_IMGS_BASE
    echo "Update hardware data in SAS_IMGS_BASE"
    run ./utils/pregen/update_hosts.py -a update -g SAS_IMGS_BASE  --ignore-unknown-lastupdate --update-older-than 5 -p --ignore-detect-fail -y
    echo "Cycle unworking hosts"
    run recluster2/prepare.sh cycle_hosts SAS_IMGS_BASE
}

function allocate_hosts() {
    echo "Generate RIM intlookups"
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_RIM -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_RIM_NIDX -c generate_intlookups
}

function recluster() {
    echo "Clearing custom instance power"
    run ./utils/common/update_card.py -g SAS_IMGS_BASE -k legacy.funcs.instanceCount -v i60g -y  # increase slot size before recluster
    run ./utils/common/clear_custom_instance_power.py -g SAS_IMGS_BASE
    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g SAS_IMGS_BASE -t optimizers/sas/configs/sas.imgs.yaml -o 18
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/sas.imgs.yaml
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/sas.imgs.yaml --max-instance-power-ratio 1000
    echo "Adding cbir intlookups"
    run ./utils/postgen/add_hamster_intlookup.py -i SAS_IMGS_BASE:2 -o SAS_IMGS_CBIR_BASE -g SAS_IMGS_CBIR_BASE
    run ./tools/recluster/main.py -a recluster -c alloc_hosts,generate_intlookups -g SAS_IMGS_CBIR_INT
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g SAS_IMGS_CBIR_BASE_HAMSTER
    run ./tools/recluster/main.py -a recluster -c alloc_hosts,generate_intlookups -g SAS_IMGS_CBIR_INT_HAMSTER
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g SAS_IMGS_CBIR_BASE_NIDX
    echo "Adding hamster intlookups"
    run ./utils/postgen/add_hamster_intlookup.py -i SAS_IMGS_BASE:2 -o SAS_IMGS_BASE_HAMSTER -g SAS_IMGS_BASE_HAMSTER
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g SAS_IMGS_BASE_NIDX
    echo "Adjusting and adding ints"
    run ./utils/postgen/add_ints2.py -a alloc_int -i SAS_IMGS_BASE -n 8 --max-ints-per-host 2
    run ./utils/postgen/add_ints2.py -a alloc_int -i SAS_IMGS_BASE_HAMSTER -n 2 --max-ints-per-host 1
    exit 1
    echo "Checking constraints"
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/sas.imgs.yaml -r brief --dispersion-limit 0.03
    run ./utils/check/check_custom_instances_power.py -g SAS_IMGS_BASE
    run ./utils/check/check_disk_size.py -c optimizers/sas/configs/sas.imgs.yaml
}

select_action cleanup allocate_hosts recluster
