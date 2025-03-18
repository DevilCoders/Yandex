#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
#    echo "Update weights of all tiers"
#    run ./tools/analyzer/analyzer.py -f update.sqlite -s optimizers/sas/configs/sas.web.yaml -t 200 -u instance_cpu_usage,host_cpu_usage
#    run ./utils/common/update_sas_optimizer_config.py -c optimizers/sas/configs/sas.web.yaml -d update.sqlite -y

    echo 'Remove SAS_WEB_BASE+slaves intlookups...'
    run ./tools/recluster/main.py -a recluster -g ALL_IMGS_MR_THUMBS -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_LARGE_THUMB_NIDX -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_LARGE_THUMB -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_THUMB_NIDX -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_THUMB -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_DISK_LUCENE -c cleanup
    run ./tools/recluster/main.py -a recluster -g ALL_IMGS_MR_THUMBS -c cleanup

    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_BASE_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_INT_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_BASE_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_INT_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_BASE_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_INT_HAMSTER -c cleanup

    run ./tools/recluster/main.py -a recluster -g SAS_WEB_CALLISTO_CAM_BASE_NIDX -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_CALLISTO_CAM_BASE -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_CALLISTO_CAM_INT -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_BASE -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_INT -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_BASE -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_INT -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_BASE -c cleanup
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_INT -c cleanup

    run ./tools/recluster/main.py -a recluster -g SAS_WEB_BASE_PRIEMKA_PORTOVM -c cleanup

    echo "Cleanup custom instance power"
    run ./utils/common/clear_custom_instance_power.py -g SAS_WEB_BASE
    echo 'Update hardware data in SAS_WEB_BASE+SAS_RESERVED'
    run utils/pregen/update_hosts.py -a updatehb -g SAS_WEB_BASE,SAS_RESERVED,SAS_RESERVED_OLD -p --ignore-group-constraints --ignore-detect-fail -y
    echo "Cycle unworking hosts"
    run recluster2/prepare.sh cycle_hosts SAS_WEB_BASE
    echo 'Update reserved hosts in Sasovo'
    run utils/common/find_potential_reserved.py -g SAS_WEB_BASE -r SAS_RESERVED -p 3. -y
}

function allocate_hosts() {
    echo "Adding intlookups for SAS_DISK_LUCENE"
    run ./utils/pregen/restore_from_olddb.py -g SAS_DISK_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g SAS_WEB_BASE -t SAS_DISK_LUCENE -x SAS_DISK_LUCENE --num-slots DiskTier0-SAS_DISK_LUCENE --add-hosts-to-group SAS_DISK_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i SAS_DISK_LUCENE
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i SAS_DISK_LUCENE

    echo "Adding intlookups for SAS_IMGS_LARGE_THUMB"
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_LARGE_THUMB -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_LARGE_THUMB_NIDX -c generate_intlookups
    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i SAS_IMGS_LARGE_THUMB

    echo "Adding intlookups for SAS_VIDEO_THUMB"
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_THUMB -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_THUMB_NIDX -c generate_intlookups
    run ./utils/common/show_replicas_count.py -i SAS_VIDEO_THUMB -j -m 3
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i SAS_VIDEO_THUMB

    echo "Adding hosts to portovm"
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_BASE_PRIEMKA_PORTOVM -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_INT_PRIEMKA_PORTOVM -c alloc_hosts
}

function recluster() {
    echo "Cleanup custom instance power"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g SAS_WEB_BASE
    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g SAS_WEB_BASE -t optimizers/sas/configs/sas.web.yaml -o 18 -e max -l "\"lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'WebTier1', shards)) == 1\""
    run ./utils/common/show_unused.py -g SAS_WEB_BASE -u
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/sas.web.yaml --min-instance-power 100
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/sas.web.yaml

    echo "Adjusting and adding ints"
    run ./utils/postgen/add_ints2.py -a alloc_int -i SAS_WEB_CALLISTO_CAM_BASE,SAS_WEB_BASE.WebTier0,SAS_WEB_BASE.PlatinumTier0,SAS_WEB_BASE.WebTier1 -n 4 --max-ints-per-host 1 --extra-instances 1000
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_BASE.WebTier0 -o 16 --process-intl2
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_BASE.WebTier1 -o 16 --process-intl2
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i SAS_WEB_BASE.WebTier0,SAS_WEB_BASE.WebTier1 -n 100
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g SAS_WEB_CALLISTO_CAM_INTL2

    echo "Move intlookups to slave groups"
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_CALLISTO_CAM_BASE -g SAS_WEB_CALLISTO_CAM_BASE --int-slave-group SAS_WEB_CALLISTO_CAM_INT
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.PlatinumTier0 -g SAS_WEB_PLATINUM_JUPITER_BASE --int-slave-group SAS_WEB_PLATINUM_JUPITER_INT
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.WebTier0 -g SAS_WEB_TIER0_JUPITER_BASE --int-slave-group SAS_WEB_TIER0_JUPITER_INT
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.WebTier1 -g SAS_WEB_TIER1_JUPITER_BASE --int-slave-group SAS_WEB_TIER1_JUPITER_INT
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g SAS_WEB_CALLISTO_CAM_BASE_NIDX

    echo "Generate hamster intlookups"
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_PLATINUM_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER1_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_INTL2_HAMSTER -c generate_intlookups

    echo "Adding priemka intlookups"
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_BASE_PRIEMKA_PORTOVM -c generate_intlookups

    echo "Clean stuff"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g SAS_WEB_BASE
    echo "Checking constraints"
    run ./utils/check/check_porto_limits.py -g SAS_WEB_BASE
    run ./utils/check/check_custom_instances_power.py -g SAS_WEB_BASE
    run ./utils/check/check_rrg_memory.py -g SAS_WEB_BASE -t WebTier0:1,WebTier1:2,PlatinumTier0:1
    run ./utils/check/check_number_of_snippets_replicas.py -c optimizers/sas/configs/sas.web.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/sas.web.yaml
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/sas.web.yaml -r brief --dispersion-limit 0.03
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/sas.web.yaml --fail-value 1000
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i SAS_VIDEO_THUMB
    run ./utils/check/check_disk_size.py -c optimizers/sas/configs/sas.web.yaml # FIXME: currently broken
}

select_action cleanup allocate_hosts recluster
