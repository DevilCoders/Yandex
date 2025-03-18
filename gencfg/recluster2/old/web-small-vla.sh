#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    # cleanup slave groups
    JUPITER_GROUPS=(
        VLA_WEB_CALLISTO_CAM_BASE
        VLA_WEB_CALLISTO_CAM_INT
        VLA_WEB_PLATINUM_JUPITER_BASE
        VLA_WEB_PLATINUM_JUPITER_INT
        VLA_WEB_TIER0_JUPITER_BASE
        VLA_WEB_TIER0_JUPITER_INT
        VLA_WEB_TIER1_JUPITER_BASE
        VLA_WEB_TIER1_JUPITER_INT
        VLA_WEB_CALLISTO_CAM_INTL2

        VLA_WEB_CALLISTO_CAM_BASE_HAMSTER
        VLA_WEB_CALLISTO_CAM_INT_HAMSTER
        VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER
        VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER
        VLA_WEB_PLATINUM_JUPITER_INT_HAMSTER
        VLA_WEB_TIER0_JUPITER_BASE_HAMSTER
        VLA_WEB_TIER0_JUPITER_INT_HAMSTER
        VLA_WEB_TIER1_JUPITER_BASE_HAMSTER
        VLA_WEB_TIER1_JUPITER_INT_HAMSTER
        VLA_WEB_INTL2_HAMSTER

        VLA_WEB_PLATINUM_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_INTL2_PIP
        VLA_WEB_PLATINUM_JUPITER_BASE_TEST1
        VLA_WEB_PLATINUM_JUPITER_INT_TEST1
        VLA_WEB_PLATINUM_JUPITER_BASE_TEST2
        VLA_WEB_PLATINUM_JUPITER_INT_TEST2
        VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB1
        VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB2
        VLA_WEB_PLATINUM_JUPITER_INT_RTHUB1
        VLA_WEB_PLATINUM_JUPITER_INT_RTHUB2

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
        VLA_PSI_DYNAMIC_AGENTS
    )

    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done

    run ./utils/common/update_igroups.py -a emptygroup -g VLA_WEB_INTL2
    # cleanup master groups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_BASE -c cleanup
}

function allocate_hosts() {
    echo "Adding intlookups for VLA_IMGS_RIM"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_RIM -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_RIM_NIDX -c generate_intlookups

    echo "Adding intlookups for JUPITER_BASE_PIP"
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_PIP -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_BASE_PIP -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_INTL2_PIP -c alloc_hosts,generate_intlookups

    echo "Adding intlookups for JUPITER_BASE_TEST1/2/RTHUB1/2"
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_TEST1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_TEST1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_TEST2 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_TEST2 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_RTHUB1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB2 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_RTHUB2 -c alloc_hosts,generate_intlookups
    
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
    run ./optimizers/sas/main.py -g VLA_WEB_BASE -t optimizers/sas/configs/vla.web.yaml -o 18 -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) <= 2 and len(filter(lambda x: x.tier_name == 'WebTier1', shards)) == 1" -e uniform
    run ./utils/common/show_replicas_count.py -i VLA_WEB_BASE.PlatinumTier0,VLA_WEB_BASE.WebTier0,VLA_WEB_BASE.WebTier1,VLA_WEB_CALLISTO_CAM_BASE,VLA_IMGS_BASE,VLA_VIDEO_BASE,VLA_ADDRS_12_BASE
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/vla.web.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/vla.web.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/vla.web.yaml
}

function recluster() {
    echo "Adding ints for production"
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_CALLISTO_CAM_BASE,VLA_WEB_BASE.PlatinumTier0,VLA_WEB_BASE.WebTier0,VLA_WEB_BASE.WebTier1 -n 4 --max-ints-per-host 2
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_VIDEO_BASE,VLA_IMGS_BASE -n 3 --max-ints-per-host 2
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_WEB_BASE.WebTier0
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_WEB_BASE.WebTier1

    echo "Move intlookups to slave groups"
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_CALLISTO_CAM_BASE -g VLA_WEB_CALLISTO_CAM_BASE --int-slave-group VLA_WEB_CALLISTO_CAM_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_BASE.PlatinumTier0 -g VLA_WEB_PLATINUM_JUPITER_BASE --int-slave-group VLA_WEB_PLATINUM_JUPITER_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_BASE.WebTier0 -g VLA_WEB_TIER0_JUPITER_BASE --int-slave-group VLA_WEB_TIER0_JUPITER_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_BASE.WebTier1 -g VLA_WEB_TIER1_JUPITER_BASE --int-slave-group VLA_WEB_TIER1_JUPITER_INT

    run ./utils/common/move_intlookup_to_slave.py -i VLA_VIDEO_BASE -g VLA_VIDEO_BASE --int-slave-group VLA_VIDEO_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_IMGS_BASE -g VLA_IMGS_BASE --int-slave-group VLA_IMGS_INT

    run ./utils/common/move_intlookup_to_slave.py -i VLA_ADDRS_12_BASE -g VLA_ADDRS_12_BASE

## sed -i -E 's/31378:[0-9]+\.[0-9]+:VLA_WEB_TIER0_JUPITER_INT/31378:77\.0:VLA_WEB_TIER0_JUPITER_INT/g' VLA_WEB_BASE.WebTier0

    echo "Add intl2"
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_WEB_INTL2 --add-hosts-to-group VLA_WEB_INTL2 --num-slots 900 -y
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_BASE.WebTier0,VLA_WEB_BASE.WebTier1 -n 100 -t VLA_WEB_INTL2
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_WEB_CALLISTO_CAM_INTL2 --add-hosts-to-group VLA_WEB_CALLISTO_CAM_INTL2 --num-slots 60 -y
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_CALLISTO_CAM_BASE -n 60
   
    echo "Adding hamster"
    JUPITER_GROUPS=(
        VLA_WEB_CALLISTO_CAM_INT_HAMSTER
        VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done

    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_CALLISTO_CAM_BASE -o VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -I
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -n 5
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -t VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER -n 20

    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER1_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER1_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_INTL2_HAMSTER -c alloc_hosts,generate_intlookups

	run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_VIDEO_BASE_HAMSTER
	run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_VIDEO_BASE_HAMSTER -n 8

    echo "Generate nidx groups"
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_VIDEO_BASE_NIDX
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_IMGS_BASE_NIDX

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
    run ./utils/check/check_number_of_snippets_replicas.py -c optimizers/sas/configs/vla.web.yaml
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/vla.web.yaml -r brief --dispersion-limit 0.05
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/vla.web.yaml --fail-value 1000
}

select_action cleanup allocate_hosts recluster
