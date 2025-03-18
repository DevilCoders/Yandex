#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
#    echo "Update weights of all tiers"
#    run ./tools/analyzer/analyzer.py -f update.sqlite -s optimizers/sas/configs/man.web.yaml -t 200 -u instance_cpu_usage,host_cpu_usage
#    run ./utils/common/update_sas_optimizer_config.py -c optimizers/sas/configs/man.web.yaml -d update.sqlite -y

    # cleanup
    CLEANUP_GROUPS=(
        # Some groups
        MAN_IMGS_THUMB_NEW_NIDX
        MAN_IMGS_MR_THUMBS
        MAN_IMGS_THUMB_NEW
        MAN_IMGS_LARGE_THUMB_NIDX
        MAN_IMGS_MR_LARGE_THUMBS
        MAN_IMGS_LARGE_THUMB
        MAN_VIDEO_MR_THUMB
        MAN_VIDEO_THUMB
        MAN_MAIL_LUCENE
        MAN_DISK_LUCENE
        
        # Hamster
        MAN_WEB_INTL2_HAMSTER
        MAN_WEB_TIER0_JUPITER_BASE_HAMSTER
        MAN_WEB_TIER0_JUPITER_INT_HAMSTER
        MAN_WEB_TIER1_JUPITER_BASE_HAMSTER
        MAN_WEB_TIER1_JUPITER_INT_HAMSTER
        MAN_WEB_PLATINUM_JUPITER_BASE_HAMSTER
        MAN_WEB_PLATINUM_JUPITER_INT_HAMSTER

        # C1
        MAN_WEB_BASE_PRIEMKA_PORTOVM
        MAN_WEB_INT_PRIEMKA_PORTOVM

        # Psi groups
        MAN_YT_PROD_PORTOVM
        MAN_YT_TESTING0_PORTOVM
        MAN_YT_TESTING1_PORTOVM
        MAN_YT_TESTING2_PORTOVM

        # Callisto
        MAN_WEB_CALLISTO_CAM_BASE
        MAN_WEB_CALLISTO_CAM_INT
        MAN_WEB_CALLISTO_CAM_INTL2

        # Platinum
        MAN_WEB_PLATINUM_JUPITER_BASE
        MAN_WEB_PLATINUM_JUPITER_INT
        MAN_WEB_PLATINUM_JUPITER_BASE_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_BUILD_X

        # Platinum night builds
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST1_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST1_BUILD_X
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST2_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST2_BUILD_X

        MAN_WEB_INTL2

        # Tier0
        MAN_WEB_TIER0_JUPITER_BASE
        MAN_WEB_TIER0_JUPITER_INT
        MAN_WEB_TIER0_JUPITER_BASE_BUILD
        MAN_WEB_TIER0_JUPITER_BASE_BUILD_X

        # Tier1
        MAN_WEB_TIER1_JUPITER_BASE
        MAN_WEB_TIER1_JUPITER_INT
        MAN_WEB_TIER1_JUPITER_BASE_BUILD
        MAN_WEB_TIER1_JUPITER_BASE_BUILD_X

        # Some groups
        MAN_WEB_FRESH_BASE_BUILD
        MAN_WEB_FRESH_BASE_BUILD_X
        MAN_WEB_GOLDEN1B_BASE_TEST1_BUILD
        MAN_WEB_GOLDEN1B_BASE_TEST1_BUILD_X
        MAN_WEB_GOLDEN1B_BASE_TEST2_BUILD
        MAN_WEB_GOLDEN1B_BASE_TEST2_BUILD_X
        MAN_WEB_IO1_BASE_TEST_BUILD
        MAN_WEB_IO1_BASE_TEST_BUILD_X
        MAN_WEB_IO2_BASE_TEST_BUILD
        MAN_WEB_IO2_BASE_TEST_BUILD_X
        MAN_WEB_JUD_JUPITER_BASE_BUILD
        MAN_WEB_JUD_JUPITER_BASE_BUILD_X
        MAN_WEB_CALLISTO_CAM_MMETA
        MAN_WEB_INTL2_PRIEMKA_PORTOVM
        MAN_WEB_TIER0_JUPITER_MMETA
        MAN_WEB_TIER1_JUPITER_MMETA
    )
    for GROUP in ${CLEANUP_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done

    run ./utils/common/reset_intlookups.py -g MAN_WEB_BASE -d
    run ./utils/common/clear_custom_instance_power.py -g MAN_WEB_BASE
    run ./utils/pregen/update_hosts.py -a updatehb -g MAN_WEB_BASE,MAN_RESERVED,MAN_RESERVED_OLD -p --ignore-group-constraints --ignore-detect-fail -y
    run ./recluster2/prepare.sh cycle_hosts MAN_WEB_BASE

        
    echo 'Update reserved hosts'
    ## ./utils/common/update_igroups.py -a emptygroup -g MAN_RESERVED -c MAN_RESERVED_OLD
    # ./utils/common/find_potential_reserved.py -g MAN_WEB_BASE -r MAN_RESERVED -o 30 -y -f "lambda x: x.memory > 500"
    run ./utils/common/find_potential_reserved.py -g MAN_WEB_BASE -r MAN_RESERVED -p 3. -y
}

function allocate_hosts() {
#    echo "Adding intlookups for MAN_MAIL_LUCENE"
#    run ./utils/pregen/restore_from_olddb.py -g MAN_MAIL_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g MAN_WEB_BASE -t MAN_MAIL_LUCENE -x MAN_MAIL_LUCENE --num-slots MailTier0-MAN_MAIL_LUCENE --add-hosts-to-group MAN_MAIL_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
#    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i MAN_MAIL_LUCENE
#    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i MAN_MAIL_LUCENE
#
#    echo "Adding intlookups for MAN_DISK_LUCENE"
#    run ./utils/pregen/restore_from_olddb.py -g MAN_DISK_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g MAN_WEB_BASE -t MAN_DISK_LUCENE -x MAN_DISK_LUCENE --num-slots DiskTier0-MAN_DISK_LUCENE --add-hosts-to-group MAN_DISK_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
#    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i MAN_DISK_LUCENE
#    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i MAN_DISK_LUCENE
#
#    echo "Adding intlookups for MAN_IMGS_THUMB_NEW"
#    run ./utils/pregen/restore_from_olddb.py -g MAN_IMGS_THUMB_NEW -r hosts,intlookups -b /home/osol/gencfg/db -v
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g MAN_WEB_BASE -t MAN_IMGS_THUMB_NEW -x MAN_IMGS_THUMB_NEW --num-slots ImtubWideTier0*4-MAN_IMGS_THUMB_NEW --add-hosts-to-group MAN_IMGS_THUMB_NEW --used-instances intlookups -y
#    run ./utils/common/add_extra_replicas.py -a addtotal -r 4 -i MAN_IMGS_THUMB_NEW
#    run ./utils/postgen/shift_intlookup.py -i MAN_IMGS_THUMB_NEW -o MAN_IMGS_THUMB_NEW_NIDX
#    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_MR_THUMBS -c generate_intlookups
#    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i MAN_IMGS_THUMB_NEW
#
#    echo "Adding intlookups for MAN_IMGS_LARGE_THUMB"
#    run ./utils/pregen/restore_from_olddb.py -g MAN_IMGS_LARGE_THUMB -r hosts,intlookups -b /home/osol/gencfg/db -v
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g MAN_WEB_BASE -t MAN_IMGS_LARGE_THUMB --num-slots ImtubLargeTier0*2-MAN_IMGS_LARGE_THUMB --add-hosts-to-group MAN_IMGS_LARGE_THUMB --used-instances intlookups -y
#    run ./utils/common/add_extra_replicas.py -a addtotal -r 2 -i MAN_IMGS_LARGE_THUMB
#    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i MAN_IMGS_LARGE_THUMB
#    run ./utils/postgen/shift_intlookup.py -i MAN_IMGS_LARGE_THUMB -o MAN_IMGS_LARGE_THUMB_NIDX
#    run ./utils/postgen/shift_intlookup.py -i MAN_IMGS_LARGE_THUMB -o MAN_IMGS_MR_LARGE_THUMBS
#
#    echo "Adding intlookups for MAN_VIDEO_THUMB"
#    run ./utils/pregen/restore_from_olddb.py -g MAN_VIDEO_THUMB -r hosts,intlookups -b /home/osol/gencfg/db -v
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g MAN_WEB_BASE -t MAN_VIDEO_THUMB -x MAN_VIDEO_THUMB --num-slots VtubTier0*4-MAN_VIDEO_THUMB --add-hosts-to-group MAN_VIDEO_THUMB --used-instances intlookups -y
#    run ./utils/common/add_extra_replicas.py -a addtotal -r 4 -i MAN_VIDEO_THUMB
#    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_MR_THUMB -c generate_intlookups
#    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i MAN_VIDEO_THUMB

    # alloc jupiter resources
    JUPITER_GROUPS=(
        MAN_WEB_CALLISTO_CAM_MMETA
        MAN_WEB_TIER0_JUPITER_MMETA
        MAN_WEB_TIER1_JUPITER_MMETA
        MAN_WEB_TIER1_JUPITER_BASE_BUILD
        MAN_WEB_TIER1_JUPITER_BASE_BUILD_X
        MAN_WEB_TIER0_JUPITER_BASE_BUILD
        MAN_WEB_TIER0_JUPITER_BASE_BUILD_X
    )

    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done

    # generate jupiter intlookups
    JUPITER_GROUPS=(
        MAN_WEB_TIER1_JUPITER_BASE_BUILD
        MAN_WEB_TIER0_JUPITER_BASE_BUILD
    )

    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c generate_intlookups
    done

    run ./utils/common/update_card.py -g MAN_WEB_BASE_PRIEMKA_PORTOVM -k reqs.instances.memory_guarantee -v "16 Gb" -y
}

function recluster() {
#    echo "Cleanup custom instance power"
#    run ./utils/common/manipulate_custom_instances_power.py -a remove -g MAN_WEB_BASE
#    echo "Generating intlookups"
#    run ./optimizers/sas/main.py -g MAN_WEB_BASE -t optimizers/sas/configs/man.web.yaml -o 18 -l "\"lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) <= 1 and len(filter(lambda x: x.tier_name == 'WebTier1', shards)) <= 1\"" -e uniform
#    run ./utils/common/show_unused.py -g MAN_WEB_BASE -u
#    run ./utils/check/check_number_of_snippets_replicas.py -c optimizers/sas/configs/man.web.yaml
#    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/man.web.yaml
#    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/man.web.yaml
#    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/man.web.yaml
#
#    echo "Adjusting and adding ints"
#    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_WEB_CALLISTO_CAM_BASE,MAN_WEB_BASE.WebTier0,MAN_WEB_BASE.PlatinumTier0,MAN_WEB_BASE.WebTier1 -n 5 --max-ints-per-host 1
#    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i MAN_WEB_BASE.WebTier0
#    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i MAN_WEB_BASE.WebTier1

#    echo "Move intlookups to slave groups"
#    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_CALLISTO_CAM_BASE -g MAN_WEB_CALLISTO_CAM_BASE --int-slave-group MAN_WEB_CALLISTO_CAM_INT
#    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.PlatinumTier0 -g MAN_WEB_PLATINUM_JUPITER_BASE --int-slave-group MAN_WEB_PLATINUM_JUPITER_INT
#    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.WebTier0 -g MAN_WEB_TIER0_JUPITER_BASE --int-slave-group MAN_WEB_TIER0_JUPITER_INT
## set TIER1 to 70
#    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.WebTier1 -g MAN_WEB_TIER1_JUPITER_BASE --int-slave-group MAN_WEB_TIER1_JUPITER_INT

#    run ./utils/common/manipulate_custom_instances_power.py -a fixfree -g MAN_WEB_BASE
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INTL2 -c alloc_hosts,generate_intlookups
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_CALLISTO_CAM_INTL2 -c alloc_hosts,generate_intlookups
#
    #HAMSTER
#    run ./utils/postgen/shift_intlookup.py -i MAN_WEB_PLATINUM_JUPITER_BASE -o MAN_WEB_PLATINUM_JUPITER_BASE_HAMSTER -I
#    run ./utils/postgen/shift_intlookup.py -i MAN_WEB_TIER0_JUPITER_BASE -o MAN_WEB_TIER0_JUPITER_BASE_HAMSTER -I
#    run ./utils/postgen/shift_intlookup.py -i MAN_WEB_TIER1_JUPITER_BASE -o MAN_WEB_TIER1_JUPITER_BASE_HAMSTER -I
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INTL2_HAMSTER -c alloc_hosts
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_PLATINUM_JUPITER_INT_HAMSTER -c alloc_hosts
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER1_JUPITER_INT_HAMSTER -c alloc_hosts
#    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_WEB_PLATINUM_JUPITER_BASE_HAMSTER -n 10
#    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_WEB_TIER0_JUPITER_BASE_HAMSTER -n 3
#    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_WEB_TIER1_JUPITER_BASE_HAMSTER -n 3
#    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i MAN_WEB_TIER0_JUPITER_BASE_HAMSTER,MAN_WEB_TIER1_JUPITER_BASE_HAMSTER -t MAN_WEB_INTL2_HAMSTER -n 10

#    echo "Adding priemka on vm"
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INT_PRIEMKA_PORTOVM -c alloc_hosts
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_BASE_PRIEMKA_PORTOVM -c generate_intlookups
#    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INTL2_PRIEMKA_PORTOVM -c alloc_hosts,generate_intlookups

    echo "More builders" 
    JUPITER_GROUPS=(
        MAN_WEB_PLATINUM_JUPITER_BASE_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_BUILD_X
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST1_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST1_BUILD_X
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST2_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST2_BUILD_X
        MAN_WEB_FRESH_BASE_BUILD
        MAN_WEB_FRESH_BASE_BUILD_X
        MAN_WEB_GOLDEN1B_BASE_TEST1_BUILD
        MAN_WEB_GOLDEN1B_BASE_TEST1_BUILD_X
        MAN_WEB_GOLDEN1B_BASE_TEST2_BUILD
        MAN_WEB_GOLDEN1B_BASE_TEST2_BUILD_X
        MAN_WEB_IO1_BASE_TEST_BUILD
        MAN_WEB_IO1_BASE_TEST_BUILD_X
        MAN_WEB_IO2_BASE_TEST_BUILD
        MAN_WEB_IO2_BASE_TEST_BUILD_X
        MAN_WEB_JUD_JUPITER_BASE_BUILD
        MAN_WEB_JUD_JUPITER_BASE_BUILD_X
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done
    
    echo "More builders intlookups" 
    JUPITER_GROUPS=(
        MAN_WEB_PLATINUM_JUPITER_BASE_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST1_BUILD
        MAN_WEB_PLATINUM_JUPITER_BASE_TEST2_BUILD
        MAN_WEB_FRESH_BASE_BUILD
        MAN_WEB_GOLDEN1B_BASE_TEST1_BUILD
        MAN_WEB_GOLDEN1B_BASE_TEST2_BUILD
        MAN_WEB_IO1_BASE_TEST_BUILD
        MAN_WEB_IO2_BASE_TEST_BUILD
        MAN_WEB_JUD_JUPITER_BASE_BUILD
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c generate_intlookups
    done

    echo "Recluster yt_prod/xcalc groups"
    run ./tools/recluster/main.py -a recluster -g MAN_YT_TESTING2_PORTOVM -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g MAN_YT_TESTING1_PORTOVM -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g MAN_YT_TESTING0_PORTOVM -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g MAN_YT_PROD_PORTOVM -c alloc_hosts

    echo "Clean stuff"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g MAN_WEB_BASE

    echo "Checking constraints"
    run ./utils/check/check_porto_limits.py -g MAN_WEB_BASE
    run ./utils/check/check_custom_instances_power.py -g MAN_WEB_BASE
    run ./utils/check/check_rrg_memory.py -g MAN_WEB_BASE -t WebTier0:1,WebTier1:2,PlatinumTier0:1
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/man.web.yaml
    run ./utils/check/check_number_of_snippets_replicas.py -c optimizers/sas/configs/man.web.yaml
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/man.web.yaml --fail-value 1000
    run ./utils/common/show_theory_host_load_distribution.py  -c optimizers/sas/configs/man.web.yaml -r brief --dispersion-limit 0.02
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i MAN_IMGS_THUMB_NEW
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i MAN_VIDEO_THUMB
    run ./utils/check/check_disk_size.py -c optimizers/sas/configs/man.web.yaml # FIXME: often broken
}

if [ "${1}" != "--source-only" ]; then
    select_action cleanup allocate_hosts recluster
fi
