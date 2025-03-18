#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    JUPITER_GROUPS=(
        VLA_WEB_PLATINUM_JUPITER_BASE
        VLA_WEB_PLATINUM_JUPITER_INT
        VLA_WEB_TIER0_JUPITER_BASE
        VLA_WEB_TIER0_JUPITER_INT
        VLA_WEB_TIER1_JUPITER_BASE
        VLA_WEB_TIER1_JUPITER_INT
        VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER
        VLA_WEB_PLATINUM_JUPITER_INT_HAMSTER
        VLA_WEB_TIER0_JUPITER_BASE_HAMSTER
        VLA_WEB_TIER0_JUPITER_INT_HAMSTER
        VLA_WEB_TIER1_JUPITER_BASE_HAMSTER
        VLA_WEB_TIER1_JUPITER_INT_HAMSTER
        VLA_WEB_INTL2_HAMSTER
        VLA_WEB_INTL2
        
        VLA_MULTIBETA1_MMETA
        VLA_MULTIINT1_MMETA
        VLA_WEB_CALLISTO_CAM_BASE
        VLA_WEB_CALLISTO_CAM_BASE_HAMSTER
        VLA_WEB_CALLISTO_CAM_INT
        VLA_WEB_CALLISTO_CAM_INTL2
        VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER
        VLA_WEB_CALLISTO_CAM_INT_HAMSTER
        VLA_WEB_GEMINI_BASE
        VLA_WEB_GEMINI_SEARCHPROXY_DYN
        VLA_WEB_JUD_JUPITER_BASE_NIDX
        VLA_WEB_MMETA_PIP
        VLA_WEB_MMETA_PRIEMKA_HAMSTER
        VLA_WEB_MMETA_PRS_HAMSTER
        VLA_WEB_PLATINUM_JUPITER_BASE_PIP
        VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB1
        VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB2
        VLA_WEB_PLATINUM_JUPITER_BASE_TEST1
        VLA_WEB_PLATINUM_JUPITER_BASE_TEST2
        VLA_WEB_PLATINUM_JUPITER_INT_PIP
        VLA_WEB_PLATINUM_JUPITER_INT_RTHUB1
        VLA_WEB_PLATINUM_JUPITER_INT_RTHUB2
        VLA_WEB_PLATINUM_JUPITER_INT_TEST1
        VLA_WEB_PLATINUM_JUPITER_INT_TEST2
        VLA_WEB_PLATINUM_JUPITER_RTHUB1_MMETA
        VLA_WEB_PLATINUM_JUPITER_RTHUB2_MMETA
        VLA_WEB_PLATINUM_JUPITER_TEST1_MMETA
        VLA_WEB_PLATINUM_JUPITER_TEST2_MMETA
        VLA_WEB_TIER0_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_INTL2_PIP
        VLA_WEB_TIER0_JUPITER_INT_PIP

        VLA_VIDEO_PLATINUM_BASE_BETA
        VLA_VIDEO_TIER0_BASE_BETA
        VLA_VIDEO_PLATINUM_INT_BETA
        VLA_VIDEO_TIER0_INT_BETA
        VLA_VIDEO_MMETA_BETA

        VLA_VIDEO_THUMB
        VLA_IMGS_THUMB_NEW_NIDX
        VLA_IMGS_THUMB_NEW
        VLA_IMGS_LARGE_THUMB_NIDX
        VLA_IMGS_LARGE_THUMB
        VLA_MAIL_LUCENE
        VLA_DISK_LUCENE
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done

}

function allocate_hosts() {
    echo "Adding intlookups for VLA_MAIL_LUCENE"
    run ./utils/pregen/restore_from_olddb.py -g VLA_MAIL_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_MAIL_LUCENE -x VLA_MAIL_LUCENE,VLA_YT_NODES,VLA_YT_NODES_SSD_JOURNALS,VLA_WEB_MEMORY2 --num-slots MailTier0-VLA_MAIL_LUCENE --add-hosts-to-group VLA_MAIL_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i VLA_MAIL_LUCENE
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i VLA_MAIL_LUCENE

    echo "Adding intlookups for VLA_DISK_LUCENE"
    run ./utils/pregen/restore_from_olddb.py -g VLA_DISK_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_DISK_LUCENE -x VLA_DISK_LUCENE,VLA_YT_NODES,VLA_YT_NODES_SSD_JOURNALS,VLA_WEB_MEMORY2 --num-slots DiskTier0-VLA_DISK_LUCENE --add-hosts-to-group VLA_DISK_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i VLA_DISK_LUCENE
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i VLA_DISK_LUCENE

    echo "Adding intlookups for VLA_VIDEO_THUMB"
    run ./utils/pregen/restore_from_olddb.py -g VLA_VIDEO_THUMB -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_VIDEO_THUMB -x VLA_VIDEO_THUMB,VLA_YT_NODES,VLA_YT_NODES_SSD_JOURNALS,VLA_WEB_MEMORY2 --num-slots VtubTier0*4-VLA_VIDEO_THUMB --add-hosts-to-group VLA_VIDEO_THUMB --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 4 -i VLA_VIDEO_THUMB --free-instances-from-intlookup
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i VLA_VIDEO_THUMB

    echo "Adding intlookups for VLA_IMGS_THUMB_NEW"
    run ./utils/pregen/restore_from_olddb.py -g VLA_IMGS_THUMB_NEW -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_IMGS_THUMB_NEW -x VLA_IMGS_THUMB_NEW,VLA_YT_NODES,VLA_YT_NODES_SSD_JOURNALS,VLA_WEB_MEMORY2 --num-slots ImtubWideTier0*3-VLA_IMGS_THUMB_NEW --add-hosts-to-group VLA_IMGS_THUMB_NEW --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 3 -i VLA_IMGS_THUMB_NEW --free-instances-from-intlookup
    run ./utils/postgen/shift_intlookup.py -i VLA_IMGS_THUMB_NEW -o VLA_IMGS_THUMB_NEW_NIDX
    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i VLA_IMGS_THUMB_NEW

    echo "Adding intlookups for VLA_IMGS_LARGE_THUMB"
    run ./utils/pregen/restore_from_olddb.py -g VLA_IMGS_LARGE_THUMB -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_IMGS_LARGE_THUMB -x VLA_IMGS_LARGE_THUMB,VLA_YT_NODES,VLA_YT_NODES_SSD_JOURNALS,VLA_WEB_MEMORY2 --num-slots ImtubLargeTier0*2-VLA_IMGS_LARGE_THUMB --add-hosts-to-group VLA_IMGS_LARGE_THUMB --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 2 -i VLA_IMGS_LARGE_THUMB
    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i VLA_IMGS_LARGE_THUMB
    run ./utils/postgen/shift_intlookup.py -i VLA_IMGS_LARGE_THUMB -o VLA_IMGS_LARGE_THUMB_NIDX

    echo "Adding intlookups for JUPITER_BASE_PIP"
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_PIP -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_BASE_PIP -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_PIP -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_INT_PIP -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_INTL2_PIP -c alloc_hosts,generate_intlookups

    echo "Adding intlookups for JUPITER_BASE_TEST1/2/RTHUB1/2/video_platinum"
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_TEST1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_TEST1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_TEST2 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_TEST2 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_RTHUB1 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB2 -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_RTHUB2 -c alloc_hosts,generate_intlookups
    
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_PLATINUM_BASE_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_TIER0_BASE_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_PLATINUM_INT_BETA -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_TIER0_INT_BETA -c alloc_hosts,generate_intlookups
   
    echo "Gemini/Jud" 
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_GEMINI_BASE -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_GEMINI_SEARCHPROXY_DYN -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_JUD_JUPITER_BASE_NIDX -c alloc_hosts,generate_intlookups

    echo "some mmetas"
    run ./tools/recluster/main.py -a recluster -g VLA_MULTIBETA1_MMETA -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_MULTIINT1_MMETA -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_MMETA_PIP -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_MMETA_PRIEMKA_HAMSTER -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_MMETA_PRS_HAMSTER -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_RTHUB1_MMETA -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_RTHUB2_MMETA -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_TEST1_MMETA -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_TEST2_MMETA -c alloc_hosts
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_MMETA_BETA -c alloc_hosts

    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g VLA_YT_RTC -t optimizers/sas/configs/arnold.yaml -o 18 -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) <= 2 and len(filter(lambda x: x.tier_name == 'WebTier1', shards)) == 1" -e uniform
    run ./utils/common/show_replicas_count.py -i VLA_WEB_CALLISTO_CAM_BASE,VLA_YT_RTC.PlatinumTier0,VLA_YT_RTC.WebTier0,VLA_YT_RTC.WebTier1
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/arnold.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/arnold.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/arnold.yaml
}

function recluster() {
#    echo "Adding ints for production"
#    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_CALLISTO_CAM_BASE,VLA_YT_RTC.PlatinumTier0,VLA_YT_RTC.WebTier0,VLA_YT_RTC.WebTier1 -n 4 --max-ints-per-host 2
#    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_YT_RTC.WebTier0
#    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_YT_RTC.WebTier1
#
#    echo "Move intlookups to slave groups"
#    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_CALLISTO_CAM_BASE -g VLA_WEB_CALLISTO_CAM_BASE --int-slave-group VLA_WEB_CALLISTO_CAM_INT
#    run ./utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.PlatinumTier0 -g VLA_WEB_PLATINUM_JUPITER_BASE --int-slave-group VLA_WEB_PLATINUM_JUPITER_INT
#    run ./utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.WebTier0 -g VLA_WEB_TIER0_JUPITER_BASE --int-slave-group VLA_WEB_TIER0_JUPITER_INT
#    run ./utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.WebTier1 -g VLA_WEB_TIER1_JUPITER_BASE --int-slave-group VLA_WEB_TIER1_JUPITER_INT
#
#    echo "Add intl2"
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_WEB_INTL2 --add-hosts-to-group VLA_WEB_INTL2 --num-slots 900 -y
#    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_YT_RTC.WebTier0,VLA_YT_RTC.WebTier1 -n 100 -t VLA_WEB_INTL2
#    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_YT_RTC -t VLA_WEB_CALLISTO_CAM_INTL2 --add-hosts-to-group VLA_WEB_CALLISTO_CAM_INTL2 --num-slots 60 -y
#    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_CALLISTO_CAM_BASE -n 60
#   
#    echo "Adding hamster"
#    JUPITER_GROUPS=(
#        VLA_WEB_CALLISTO_CAM_INT_HAMSTER
#        VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER
#    )
#    for GROUP in ${JUPITER_GROUPS[@]}; do
#        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
#    done
#
#    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_CALLISTO_CAM_BASE -o VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -I
#    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -n 5
#    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -t VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER -n 20
#
#    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER -c generate_intlookups
#    run ./tools/recluster/main.py -a recluster -g VLA_WEB_PLATINUM_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
#    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER1_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_TIER1_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_INTL2_HAMSTER -c alloc_hosts,generate_intlookups

    echo "Clean stuff"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g VLA_YT_RTC

    echo "Checking constraints"
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/arnold.yaml -r brief --dispersion-limit 0.05
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/arnold.yaml --fail-value 1000
}

select_action cleanup allocate_hosts recluster
