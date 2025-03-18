#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
#    echo "Update weights of all tiers"
#    run ./tools/analyzer/analyzer.py -f update.sqlite -s optimizers/sas/configs/vla.major.yaml -t 200 -u instance_cpu_usage,host_cpu_usage
#    run ./utils/common/update_sas_optimizer_config.py -c optimizers/sas/configs/vla.major.yaml -d update.sqlite -y

    # cleanup non-web groups
    run ./tools/recluster/main.py -a recluster -g VLA_VIDEO_THUMB -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_THUMB_NEW_NIDX -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_THUMB_NEW -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_LARGE_THUMB_NIDX -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_LARGE_THUMB -c cleanup

    # cleanup slave groups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_BASE_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_INT_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_INTL2_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_CALLISTO_CAM_INT_HAMSTER -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER -c cleanup

    # cleanup lucene
    run ./tools/recluster/main.py -a recluster -g VLA_MAIL_LUCENE -c cleanup
    run ./tools/recluster/main.py -a recluster -g VLA_DISK_LUCENE -c cleanup

    JUPITER_GROUPS=(
        # Imgs C1
        VLA_IMGS_BASE_C1
        VLA_IMGS_INT_C1

        # Cbir priemka
        VLA_IMGS_CBIR_BASE_PRIEMKA
        VLA_IMGS_CBIR_INT_PRIEMKA

        # Ganymede
        VLA_WEB_GANYMEDE_INTL2
        VLA_WEB_GANYMEDE_PLATINUM_BASE
        VLA_WEB_GANYMEDE_PLATINUM_INT
        VLA_WEB_GANYMEDE_TIER0_BASE
        VLA_WEB_GANYMEDE_TIER0_INT

        # Video hamster
        VLA_VIDEO_BASE_HAMSTER

        # Video
        VLA_VIDEO_BASE_NIDX
        VLA_VIDEO_BASE
        VLA_VIDEO_INT

        # Imgs CBIR
        VLA_IMGS_CBIR_BASE_NIDX
        VLA_IMGS_CBIR_BASE

        # Imgs RIM
        VLA_IMGS_RIM_NIDX
        VLA_IMGS_RIM

        # ImgTier0
        VLA_IMGS_BASE_NIDX
        VLA_IMGS_BASE
        VLA_IMGS_INT

        # Callisto
        VLA_WEB_CALLISTO_CAM_BASE_NIDX
        VLA_WEB_CALLISTO_CAM_BASE
        VLA_WEB_CALLISTO_CAM_INT

        # Platinum
        VLA_WEB_PLATINUM_JUPITER_BASE
        VLA_WEB_PLATINUM_JUPITER_INT

        # Platinum PIP
        VLA_WEB_PLATINUM_JUPITER_BASE_PIP_NIDX
        VLA_WEB_PLATINUM_JUPITER_BASE_PIP

        # Tier0
        VLA_WEB_TIER0_JUPITER_BASE
        VLA_WEB_TIER0_JUPITER_INT

        # TIer0 PIP
        VLA_WEB_TIER0_JUPITER_BASE_PIP_NIDX
        VLA_WEB_TIER0_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_INTL2_PIP

        # Tier1
        VLA_WEB_TIER1_JUPITER_BASE
        VLA_WEB_TIER1_JUPITER_INT
    )

    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done

    # cleanup master groups
    run ./tools/recluster/main.py -a recluster -g VLA_WEB_BASE -c cleanup

    echo "Cycle unworking hosts"
    run ./recluster2/prepare.sh cycle_hosts VLA_WEB_BASE

    echo 'Update reserved hosts in Msk'
    run ./utils/common/find_potential_reserved.py -g VLA_WEB_BASE -r VLA_RESERVED -p 3. -y
}

function allocate_hosts() {
    echo "Allocating hosts for Ganymede"
    JUPITER_GROUPS=(
        VLA_WEB_GANYMEDE_PLATINUM_BASE
        VLA_WEB_GANYMEDE_TIER0_BASE
        VLA_WEB_GANYMEDE_PLATINUM_INT
        VLA_WEB_GANYMEDE_TIER0_INT
        VLA_WEB_GANYMEDE_INTL2
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done

    echo "Generating intlookups for Ganymede"
    JUPITER_GROUPS=(
        VLA_WEB_GANYMEDE_PLATINUM_BASE
        VLA_WEB_GANYMEDE_TIER0_BASE
        VLA_WEB_GANYMEDE_PLATINUM_INT
        VLA_WEB_GANYMEDE_TIER0_INT
        VLA_WEB_GANYMEDE_INTL2
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c generate_intlookups
    done

    echo "Adding intlookups for VLA_MAIL_LUCENE"
    run ./utils/pregen/restore_from_olddb.py -g VLA_MAIL_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_MAIL_LUCENE -x VLA_MAIL_LUCENE --num-slots MailTier0-VLA_MAIL_LUCENE --add-hosts-to-group VLA_MAIL_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i VLA_MAIL_LUCENE
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i VLA_MAIL_LUCENE

    echo "Adding intlookups for VLA_DISK_LUCENE"
    run ./utils/pregen/restore_from_olddb.py -g VLA_DISK_LUCENE -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_DISK_LUCENE -x VLA_DISK_LUCENE --num-slots DiskTier0-VLA_DISK_LUCENE --add-hosts-to-group VLA_DISK_LUCENE -f "lambda x: x.ssd == 0 or x.ssd > 800" --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 1 -i VLA_DISK_LUCENE
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i VLA_DISK_LUCENE

    echo "Adding intlookups for VLA_IMGS_CBIR_BASE"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE_NIDX -c generate_intlookups

    echo "Adding intlookups for VLA_IMGS_CBIR_BASE_PRIEMKA"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_BASE_PRIEMKA -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_CBIR_INT_PRIEMKA -c alloc_hosts,generate_intlookups

    echo "Adding intlookups for VLA_IMGS_RIM"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_RIM -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_RIM_NIDX -c generate_intlookups

    echo "Adding intlookups for VLA_VIDEO_THUMB"
    run ./utils/pregen/restore_from_olddb.py -g VLA_VIDEO_THUMB -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_VIDEO_THUMB -x VLA_VIDEO_THUMB --num-slots VtubTier0*4-VLA_VIDEO_THUMB --add-hosts-to-group VLA_VIDEO_THUMB --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 4 -i VLA_VIDEO_THUMB --free-instances-from-intlookup
    run ./utils/common/compare_intlookup_shard_hosts.py -o ~/work/gencfg.svn/db -n ./db -i VLA_VIDEO_THUMB

    echo "Adding intlookups for VLA_IMGS_THUMB_NEW"
    run ./utils/pregen/restore_from_olddb.py -g VLA_IMGS_THUMB_NEW -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_IMGS_THUMB_NEW -x VLA_IMGS_THUMB_NEW --num-slots ImtubWideTier0*3-VLA_IMGS_THUMB_NEW --add-hosts-to-group VLA_IMGS_THUMB_NEW --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 3 -i VLA_IMGS_THUMB_NEW --free-instances-from-intlookup
    run ./utils/postgen/shift_intlookup.py -i VLA_IMGS_THUMB_NEW -o VLA_IMGS_THUMB_NEW_NIDX
    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i VLA_IMGS_THUMB_NEW

    echo "Adding intlookups for VLA_IMGS_LARGE_THUMB"
    run ./utils/pregen/restore_from_olddb.py -g VLA_IMGS_LARGE_THUMB -r hosts,intlookups -b /home/osol/gencfg/db -v
    run ./utils/pregen/find_most_memory_free_machines.py -a alloc -g VLA_WEB_BASE -t VLA_IMGS_LARGE_THUMB --num-slots ImtubLargeTier0*2-VLA_IMGS_LARGE_THUMB --add-hosts-to-group VLA_IMGS_LARGE_THUMB --used-instances intlookups -y
    run ./utils/common/add_extra_replicas.py -a addtotal -r 2 -i VLA_IMGS_LARGE_THUMB
    run ./utils/common/compare_intlookup_shard_hosts.py -o /home/osol/gencfg/db -n ./db -i VLA_IMGS_LARGE_THUMB
    run ./utils/postgen/shift_intlookup.py -i VLA_IMGS_LARGE_THUMB -o VLA_IMGS_LARGE_THUMB_NIDX

    echo "Allocating hosts for PIP"
    JUPITER_GROUPS=(
        VLA_WEB_PLATINUM_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_INTL2_PIP
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done

    echo "Generating intlookups for PIP"
    JUPITER_GROUPS=(
        VLA_WEB_PLATINUM_JUPITER_BASE_PIP
        VLA_WEB_PLATINUM_JUPITER_BASE_PIP_NIDX
        VLA_WEB_TIER0_JUPITER_BASE_PIP
        VLA_WEB_TIER0_JUPITER_BASE_PIP_NIDX
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c generate_intlookups
    done

    echo "Allocating hosts for some groups"
    run ./tools/recluster/main.py -g VLA_WEB_INT_HAMSTER -a recluster -c alloc_hosts
    run ./tools/recluster/main.py -g VLA_WEB_INTL2_HAMSTER -a recluster -c alloc_hosts
    run ./tools/recluster/main.py -g VLA_WEB_CALLISTO_CAM_INT_HAMSTER -a recluster -c alloc_hosts
    run ./tools/recluster/main.py -g VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER -a recluster -c alloc_hosts
}

function recluster() {
    echo "Clearing custom instances power"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g VLA_WEB_BASE
    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g VLA_WEB_BASE -t optimizers/sas/configs/vla.major.yaml -o 36 -e uniform
    run ./utils/common/show_unused.py -g VLA_WEB_BASE -u
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/vla.major.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/vla.major.yaml

    echo "Adjusting number of instances per group"
    run ./utils/common/flat_intlookup.py -a flat -i VLA_WEB_CALLISTO_CAM_BASE
    run ./utils/common/update_intlookup.py -a changetier -t CallistoSlotsTier0 -s VLA_WEB_CALLISTO_CAM_BASE -y
    run ./utils/common/flat_intlookup.py -a unflat -i VLA_WEB_CALLISTO_CAM_BASE -o 18
    run ./utils/common/flat_intlookup.py -a flat -i VLA_WEB_BASE.PlatinumTier0
    run ./utils/common/flat_intlookup.py -a unflat -i VLA_WEB_BASE.PlatinumTier0 -o 18
    run ./utils/common/flat_intlookup.py -a flat -i VLA_WEB_BASE.WebTier0
    run ./utils/common/flat_intlookup.py -a unflat -i VLA_WEB_BASE.WebTier0 -o 18
    run ./utils/common/flat_intlookup.py -a flat -i VLA_WEB_BASE.WebTier1
    run ./utils/common/flat_intlookup.py -a unflat -i VLA_WEB_BASE.WebTier1 -o 18

    echo "Adding hamster"
    run ./utils/postgen/add_hamster_intlookup.py -i VLA_WEB_BASE.PlatinumTier0:2,VLA_WEB_BASE.WebTier0:2,VLA_WEB_BASE.WebTier1:1 -g VLA_WEB_BASE_HAMSTER
    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_CALLISTO_CAM_BASE -o VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -I
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_VIDEO_BASE_HAMSTER

    echo "Adding ints for production"
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_CALLISTO_CAM_BASE,VLA_WEB_BASE.PlatinumTier0,VLA_WEB_BASE.WebTier0,VLA_WEB_BASE.WebTier1,VLA_VIDEO_BASE,VLA_IMGS_BASE -n 5 --max-ints-per-host 2
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_WEB_BASE.WebTier0
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_WEB_BASE.WebTier1
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_BASE.WebTier0,VLA_WEB_BASE.WebTier1 -n 100
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_CALLISTO_CAM_BASE -n 20

    echo "Adding ints for hamster"
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_BASE_HAMSTER -n 8
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -n 8
    run ./utils/postgen/add_ints2.py -a alloc_int -i VLA_VIDEO_BASE_HAMSTER -n 8
    run ./utils/common/update_intlookup.py -a split -s VLA_WEB_BASE_HAMSTER -y
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_WEB_BASE_HAMSTER.WebTier0
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_WEB_BASE_HAMSTER.WebTier1
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_BASE_HAMSTER.WebTier0,VLA_WEB_BASE_HAMSTER.WebTier1 -n 10
    run ./utils/postgen/add_ints2.py -a alloc_intl2 -i VLA_WEB_CALLISTO_CAM_BASE_HAMSTER -n 10

    echo "Move intlookups to slave groups"
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_CALLISTO_CAM_BASE -g VLA_WEB_CALLISTO_CAM_BASE --int-slave-group VLA_WEB_CALLISTO_CAM_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_BASE.PlatinumTier0 -g VLA_WEB_PLATINUM_JUPITER_BASE --int-slave-group VLA_WEB_PLATINUM_JUPITER_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_BASE.WebTier0 -g VLA_WEB_TIER0_JUPITER_BASE --int-slave-group VLA_WEB_TIER0_JUPITER_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_WEB_BASE.WebTier1 -g VLA_WEB_TIER1_JUPITER_BASE --int-slave-group VLA_WEB_TIER1_JUPITER_INT
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_WEB_CALLISTO_CAM_INTL2

    run ./utils/common/move_intlookup_to_slave.py -i VLA_VIDEO_BASE -g VLA_VIDEO_BASE --int-slave-group VLA_VIDEO_INT
    run ./utils/common/move_intlookup_to_slave.py -i VLA_IMGS_BASE -g VLA_IMGS_BASE --int-slave-group VLA_IMGS_INT

    echo "Generate nidx groups"
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_WEB_CALLISTO_CAM_BASE_NIDX
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_VIDEO_BASE_NIDX
    run ./tools/recluster/main.py -a recluster -c generate_intlookups -g VLA_IMGS_BASE_NIDX

    echo "Add intlookups for Imgs C1"
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_BASE_C1 -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g VLA_IMGS_INT_C1 -c alloc_hosts,generate_intlookups

    echo "Clean stuff"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g VLA_WEB_BASE

    echo "Checking constraints"
    run ./utils/check/check_porto_limits.py -g VLA_WEB_BASE
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/vla.major.yaml
    run ./utils/check/check_number_of_snippets_replicas.py -c optimizers/sas/configs/vla.major.yaml
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/vla.major.yaml -r brief --dispersion-limit 0.05
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/vla.major.yaml --fail-value 1000
}

select_action cleanup allocate_hosts recluster
