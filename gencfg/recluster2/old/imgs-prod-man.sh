#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    echo "Remove MAN_IMGS_BASE+slaves intlookups"
    run ./utils/common/reset_intlookups.py -g MAN_IMGS_BASE,MAN_IMGS_CBIR_BASE,MAN_IMGS_BASE_NIDX,MAN_IMGS_BASE_HAMSTER,MAN_IMGS_CBIR_BASE_HAMSTER,MAN_IMGS_CBIR_BASE_NIDX -d
    echo "Cleanup custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g MAN_IMGS_BASE
}

function allocate_hosts() {
    echo "Optimization"
    run ./optimizers/sas/main.py -g MAN_IMGS_BASE -t optimizers/sas/configs/man.imgs.yaml -o 18
    run ./utils/common/show_replicas_count.py -i MAN_IMGS_BASE
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/man.imgs.yaml
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/man.imgs.yaml --max-instance-power-ratio 1000
}

function recluster() {
    echo "Adding cbir intlookups"
    run ./utils/postgen/shift_intlookup.py -i MAN_IMGS_BASE -o MAN_IMGS_CBIR_BASE -I
    run ./tools/recluster/main.py -a recluster -c alloc_hosts,generate_intlookups -g MAN_IMGS_CBIR_INT
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g MAN_IMGS_CBIR_BASE_HAMSTER
    run ./tools/recluster/main.py -a recluster -c alloc_hosts,generate_intlookups -g MAN_IMGS_CBIR_INT_HAMSTER
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g MAN_IMGS_CBIR_BASE_NIDX
    echo "Adding hamster intlookups"
    run ./utils/postgen/shift_intlookup.py -i MAN_IMGS_BASE -o MAN_IMGS_BASE_HAMSTER -I
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g MAN_IMGS_BASE_NIDX
    echo "Adjusting and adding ints"
    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_IMGS_BASE -n 8 --max-ints-per-host 1
    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_IMGS_BASE_HAMSTER -n 2 --max-ints-per-host 1
    echo "Checking constraints"
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/man.imgs.yaml -r brief --dispersion-limit 0.03
    run ./utils/check/check_custom_instances_power.py -g MAN_IMGS_BASE
}

select_action cleanup allocate_hosts recluster
