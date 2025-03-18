#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    # "instanceCount": "video_with_slaves" for MAN_VIDEO_BASE

    CLEANUP_GROUPS=(
        MAN_VIDEO_PLATINUM_BASE
        MAN_VIDEO_PLATINUM_INT
        MAN_VIDEO_PLATINUM_BASE_HAMSTER
        MAN_VIDEO_PLATINUM_INT_HAMSTER
        
        MAN_VIDEO_TIER0_BASE
        MAN_VIDEO_TIER0_INT
        MAN_VIDEO_TIER0_BASE_HAMSTER
        MAN_VIDEO_TIER0_INT_HAMSTER
    )

    for GROUP in ${CLEANUP_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
    done
    
    run ./utils/common/clear_custom_instance_power.py -g MAN_VIDEO_BASE
}

function allocate_hosts() {
    run ./optimizers/sas/main.py -g MAN_VIDEO_BASE -t optimizers/sas/configs/man.video.yaml -o 18 -e uniform
    run ./utils/common/show_replicas_count.py -i MAN_VIDEO_BASE.VideoPlatinum,MAN_VIDEO_BASE.VideoTier0
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/man.video.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/man.video.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/man.video.yaml
}

function recluster() {
    run ./utils/postgen/add_ints2.py -a alloc_int -i MAN_VIDEO_BASE.VideoPlatinum,MAN_VIDEO_BASE.VideoTier0 -n 4 --max-ints-per-host 2
    
    run ./utils/common/move_intlookup_to_slave.py -i MAN_VIDEO_BASE.VideoPlatinum -g MAN_VIDEO_PLATINUM_BASE --int-slave-group MAN_VIDEO_PLATINUM_INT
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_PLATINUM_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_PLATINUM_INT_HAMSTER -c alloc_hosts,generate_intlookups

    run ./utils/common/move_intlookup_to_slave.py -i MAN_VIDEO_BASE.VideoTier0 -g MAN_VIDEO_TIER0_BASE --int-slave-group MAN_VIDEO_TIER0_INT
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_TIER0_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_TIER0_INT_HAMSTER -c alloc_hosts,generate_intlookups

    run ./utils/common/manipulate_custom_instances_power.py -a remove -g MAN_VIDEO_BASE
    run ./utils/check/check_custom_instances_power.py -g MAN_VIDEO_BASE
    run ./utils/check/check_disk_size.py -c optimizers/sas/configs/man.video.yaml
    run ./utils/common/show_theory_host_load_distribution.py -c optimizers/sas/configs/man.video.yaml -r brief --dispersion-limit 0.05
    run ./utils/postgen/adjust_replicas.py -a show -c optimizers/sas/configs/man.video.yaml --fail-value 1000

    # "instanceCount": "exactly1" for MAN_VIDEO_BASE
}

select_action cleanup allocate_hosts recluster
