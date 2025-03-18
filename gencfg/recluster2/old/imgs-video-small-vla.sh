#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    # cleanup slave groups
    JUPITER_GROUPS=(
        VLA_IMGS_BASE_MULTI
        VLA_IMGS_INT_MULTI
        VLA_IMGS_CBIR_BASE_MULTI
        VLA_IMGS_CBIR_INT_MULTI
        VLA_IMGS_BASE_C1
        VLA_IMGS_INT_C1
        VLA_IMGS_CBIR_BASE_PRIEMKA
        VLA_IMGS_CBIR_INT_PRIEMKA
        VLA_IMGS_CBIR_BASE_NIDX
        VLA_IMGS_CBIR_BASE
        VLA_IMGS_RIM_NIDX
        VLA_IMGS_RIM
        VLA_IMGS_BASE_NIDX
        VLA_IMGS_BASE
        VLA_IMGS_INT
        VLA_IMGS_BASE_BETA
        VLA_IMGS_CBIR_BASE_BETA
        VLA_IMGS_CBIR_BASE_PRIEMKA
        VLA_IMGS_CBIR_INT_BETA
        VLA_IMGS_INT_BETA

        VLA_VIDEO_BASE_HAMSTER
        VLA_VIDEO_BASE_NIDX
        VLA_VIDEO_BASE
        VLA_VIDEO_INT
        VLA_VIDEO_BASE_BETA
        VLA_VIDEO_INT_BETA

        VLA_PSI_YT_1_PORTOVM
        VLA_PSI_YT_2_PORTOVM
    )

    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done
}

function allocate_hosts() {
    echo "Adding intlookups for VLA_IMGS_RIM"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_RIM -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_RIM_NIDX -c generate_intlookups

    echo "Adding intlookups for IMGS/CBIR_BETA"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_BASE_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_INT_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_INT_BETA -c alloc_hosts,generate_intlookups

    echo "Adding intlookups for VIDEO_BETA"
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_BASE_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_INT_BETA -c alloc_hosts,generate_intlookups

    echo "Clearing custom instances power"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g VLA_WEB_BASE
    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g VLA_IMGS_BASE_HOLDER -t optimizers/sas/configs/vla.imgs.video.yaml -o 18 -e uniform
    run ./utils/common/show_replicas_count.py -i VLA_IMGS_BASE,VLA_VIDEO_BASE
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/vla.imgs.video.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/vla.imgs.video.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/vla.imgs.video.yaml
}

function recluster() {
    run ./utils/common/move_intlookup_to_slave.py -i VLA_VIDEO_BASE -g VLA_VIDEO_BASE --int-slave-group VLA_VIDEO_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_IMGS_BASE -g VLA_IMGS_BASE --int-slave-group VLA_IMGS_INT

 	run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_VIDEO_BASE_HAMSTER
	run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_VIDEO_BASE_HAMSTER -n 8

    echo "Generate nidx groups"
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_VIDEO_BASE_NIDX
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_IMGS_BASE_NIDX

    echo "Generate multi groups"
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_IMGS_BASE_MULTI
    run ./tools/recluster/main.py -a recluster -c alloc_hosts,generate_intlookups -g VLA_IMGS_INT_MULTI
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_IMGS_CBIR_BASE_MULTI
    run ./tools/recluster/main.py -a recluster -c alloc_hosts,generate_intlookups -g VLA_IMGS_CBIR_INT_MULTI


    echo "Adding intlookups for VLA_IMGS_CBIR_BASE"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE_NIDX -c generate_intlookups

    echo "Adding intlookups for VLA_IMGS_CBIR_BASE_PRIEMKA"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE_PRIEMKA -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_INT_PRIEMKA -c alloc_hosts,generate_intlookups

    echo "Add intlookups for Imgs C1"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_BASE_C1 -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_INT_C1 -c alloc_hosts,generate_intlookups

    echo "Add PSI"
    run ./tools/recluster/main.py -a recluster -g VLA_PSI_YT_1_PORTOVM -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_PSI_YT_2_PORTOVM -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_PSI_DYNAMIC -c alloc_hosts

    echo "Clean stuff"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g VLA_WEB_BASE

    echo "Checking constraints"
    run ./utils/check/check_porto_limits.py -g VLA_WEB_BASE
    run ./utils/check/check_number_of_snippets_replicas.py -c optimizers/sas/configs/vla.imgs.video.yaml
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/vla.imgs.video.yaml -r brief --dispersion-limit 0.05
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/vla.imgs.video.yaml --fail-value 1000
}

select_action cleanup allocate_hosts recluster
