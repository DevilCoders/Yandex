#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    CLEANUP_GROUPS=(
        SAS_WEB_PLATINUM_JUPITER_INT_HAMSTER
        SAS_WEB_PLATINUM_JUPITER_BASE_HAMSTER
        SAS_WEB_TIER0_JUPITER_INT_HAMSTER
        SAS_WEB_TIER0_JUPITER_BASE_HAMSTER
        SAS_WEB_TIER1_JUPITER_INT_HAMSTER
        SAS_WEB_TIER1_JUPITER_BASE_HAMSTER

        SAS_WEB_CALLISTO_CAM_INT
        SAS_WEB_CALLISTO_CAM_BASE
        SAS_WEB_PLATINUM_JUPITER_INT
        SAS_WEB_PLATINUM_JUPITER_BASE
        SAS_WEB_TIER0_JUPITER_INT
        SAS_WEB_TIER0_JUPITER_BASE
        SAS_WEB_TIER1_JUPITER_INT
        SAS_WEB_TIER1_JUPITER_BASE

        SAS_PSI_DYNAMIC
        SAS_YT_PROD_PORTOVM
        SAS_YT_PROD2_PORTOVM
        SAS_YT_PROD3_PORTOVM
    )
    for GROUP in ${CLEANUP_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done

    echo "Cleanup custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g SAS_WEB_BASE
}

function allocate_hosts() {
#    echo "Cleanup custom instance power"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g SAS_WEB_BASE
# do all host transfers
#    exit 0
    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g SAS_WEB_BASE -t optimizers/sas/configs/sas.web.yaml -o 18 -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) <= 2 and len(filter(lambda x: x.tier_name == 'WebTier1', shards)) == hgroup.max_tier1_allowed()" -e uniform -f "lambda x: x.ssd > 1" --exclude-cpu-from SAS_IMGS_SAAS_QUICK_BASE
    run ./utils/common/show_replicas_count.py -i SAS_WEB_CALLISTO_CAM_BASE,SAS_WEB_BASE.WebTier0,SAS_WEB_BASE.PlatinumTier0,SAS_WEB_BASE.WebTier1
# check replicas
#    exit 0
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/sas.web.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/sas.web.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/sas.web.yaml
}

function recluster() {
    echo "Adjusting and adding ints"
    run ./utils/postgen/add_ints2.py -a alloc_int -i SAS_WEB_CALLISTO_CAM_BASE,SAS_WEB_BASE.WebTier0,SAS_WEB_BASE.PlatinumTier0,SAS_WEB_BASE.WebTier1 -n 5 --max-ints-per-host 1
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_BASE.WebTier0 -o 16 --process-intl2
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_BASE.WebTier1 -o 16 --process-intl2

    exit 0
    echo "Move intlookups to slave groups"
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_CALLISTO_CAM_BASE -g SAS_WEB_CALLISTO_CAM_BASE --int-slave-group SAS_WEB_CALLISTO_CAM_INT
    run sed -i 's/:0\.0:SAS_WEB_CALLISTO_CAM_INT/:39\.0:SAS_WEB_CALLISTO_CAM_INT/g' db/intlookups/SAS_WEB_CALLISTO_CAM_BASE

    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.PlatinumTier0 -g SAS_WEB_PLATINUM_JUPITER_BASE --int-slave-group SAS_WEB_PLATINUM_JUPITER_INT
    run sed -i 's/:0\.0:SAS_WEB_PLATINUM_JUPITER_INT/:68\.0:SAS_WEB_PLATINUM_JUPITER_INT/g' db/intlookups/SAS_WEB_BASE.PlatinumTier0

    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.WebTier0 -g SAS_WEB_TIER0_JUPITER_BASE --int-slave-group SAS_WEB_TIER0_JUPITER_INT
    run sed -i 's/:0\.0:SAS_WEB_TIER0_JUPITER_INT/:174\.0:SAS_WEB_TIER0_JUPITER_INT/g' db/intlookups/SAS_WEB_BASE.WebTier0
    
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.WebTier1 -g SAS_WEB_TIER1_JUPITER_BASE --int-slave-group SAS_WEB_TIER1_JUPITER_INT
    run sed -i 's/:0\.0:SAS_WEB_TIER1_JUPITER_INT/:160\.0:SAS_WEB_TIER1_JUPITER_INT/g' db/intlookups/SAS_WEB_BASE.WebTier1

    exit 0
    echo "Add intl2"
    run ./utils/common/manipulate_custom_instances_power.py -a fixfree -g SAS_WEB_BASE
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g SAS_WEB_BASE -t SAS_WEB_INTL2 --add-hosts-to-group SAS_WEB_INTL2 --num-slots 900 -y
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i SAS_WEB_BASE.WebTier0,SAS_WEB_BASE.WebTier1 -n 100 -t SAS_WEB_INTL2
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g SAS_WEB_BASE -t SAS_WEB_CALLISTO_CAM_INTL2 --add-hosts-to-group SAS_WEB_CALLISTO_CAM_INTL2 --num-slots 60 -y
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i SAS_WEB_CALLISTO_CAM_BASE -n 60
   
    echo "Generate hamster intlookups"
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_INTL2_HAMSTER -c alloc_hosts,generate_intlookups

    echo "Clean stuff"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g SAS_WEB_BASE
    echo "Checking constraints"
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/sas.web.yaml -r brief --dispersion-limit 0.03
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/sas.web.yaml --fail-value 1000
}

select_action cleanup allocate_hosts recluster
