#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    CLEANUP_GROUPS=(
        # web
        #MAN_WEB_INTL2_MULTI
       # MAN_WEB_INTL2_HAMSTER
       # MAN_WEB_INTL2
#
#        MAN_WEB_PLATINUM_JUPITER_INT_MULTI
#        MAN_WEB_PLATINUM_JUPITER_INT_HAMSTER
#        MAN_WEB_PLATINUM_JUPITER_INT
#        MAN_WEB_PLATINUM_JUPITER_BASE_MULTI
#        MAN_WEB_PLATINUM_JUPITER_BASE_HAMSTER
#        MAN_WEB_PLATINUM_JUPITER_BASE
#
#        MAN_WEB_TIER0_JUPITER_INT_MULTI
#        MAN_WEB_TIER0_JUPITER_INT_HAMSTER
#        MAN_WEB_TIER0_JUPITER_INT
#        MAN_WEB_TIER0_JUPITER_BASE_MULTI
#        MAN_WEB_TIER0_JUPITER_BASE_HAMSTER
#        MAN_WEB_TIER0_JUPITER_BASE
#
        # video
#        MAN_VIDEO_PLATINUM_INT_HAMSTER
#        MAN_VIDEO_PLATINUM_INT
#        MAN_VIDEO_PLATINUM_BASE_HAMSTER
#        MAN_VIDEO_PLATINUM_BASE
#
#        MAN_VIDEO_TIER0_INT_HAMSTER
#        MAN_VIDEO_TIER0_INT
#        MAN_VIDEO_TIER0_BASE_HAMSTER
#        MAN_VIDEO_TIER0_BASE
#
        # images
        MAN_IMGS_CBIR_INT_HAMSTER
        MAN_IMGS_CBIR_INT
        MAN_IMGS_INT_HAMSTER
        MAN_IMGS_INT
        MAN_IMGS_CBIR_BASE_HAMSTER
        MAN_IMGS_CBIR_BASE_NIDX
        MAN_IMGS_CBIR_BASE
        MAN_IMGS_BASE_HAMSTER
        MAN_IMGS_BASE_NIDX
        # MAN_IMGS_BASE

        # non-optimized images
        # MAN_IMGS_RIM
        MAN_IMGS_T1_BASE
        MAN_IMGS_T1_BASE_NIDX
        MAN_IMGS_T1_CBIR_BASE
         MAN_IMGS_T1_CBIR_BASE_NIDX
         MAN_IMGS_T1_INT
         MAN_IMGS_T1_CBIR_INT

        # psi
        MAN_YT_PROD_PORTOVM
        MAN_YT_TESTING0_PORTOVM
        MAN_YT_TESTING1_PORTOVM
        MAN_YT_TESTING2_PORTOVM
    )
    for GROUP in ${CLEANUP_GROUPS[@]}; do
        echo "cleanup ${GROUP}"
        run ./utils/common/update_igroups.py -a emptygroup -g ${GROUP}
    done
}

function allocate_hosts() {
    echo "Setup slots"
    run ./utils/common/update_card.py -g MAN_WEB_BASE -k legacy.funcs.instanceCount -v man_slot_count -y
    run ./utils/common/update_card.py -g MAN_WEB_BASE -k legacy.funcs.instancePower -v fullhost -y

    echo "Optimize it"
    run ./optimizers/sas/main.py -g MAN_WEB_BASE -t optimizers/sas/configs/man.web-video.yaml -o 18 \
        -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) <= 1" \
        -e uniform \
        --exclude-cpu-from MAN_IMGS_SAAS_QUICK_BASE,MAN_WEB_CALLISTO_CAM_BASE,MAN_IMGS_PPL_BASE \
        --exclude-hosts-from MAN_PORTAL_ANY_STABLE,MAN_PORTAL_MORDA,MAN_PORTAL_MORDA_YARU_STABLE,MAN_ADDRS_18SHARDS_BASE
    run ./utils/common/show_replicas_count.py -i MAN_WEB_BASE.WebTier0,MAN_WEB_BASE.PlatinumTier0,MAN_WEB_BASE.VideoPlatinum,MAN_WEB_BASE.VideoTier0,MAN_WEB_BASE.ImgTier0
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/man.web-video.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/man.web-video.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/man.web-video.yaml

    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i MAN_WEB_BASE.WebTier0
    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.PlatinumTier0 -g MAN_WEB_PLATINUM_JUPITER_BASE
    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.WebTier0 -g MAN_WEB_TIER0_JUPITER_BASE
    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.VideoPlatinum -g MAN_VIDEO_PLATINUM_BASE
    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.VideoTier0 -g MAN_VIDEO_TIER0_BASE
    run ./utils/common/move_intlookup_to_slave.py -i MAN_WEB_BASE.ImgTier0 -g MAN_IMGS_BASE
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_CBIR_BASE -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_BASE_NIDX -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_CBIR_BASE_NIDX -c generate_intlookups

    echo "Remove slots"
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g MAN_WEB_BASE
    run ./utils/common/update_card.py -g MAN_WEB_BASE -k legacy.funcs.instancePower -v exactly0 -y
    run ./utils/common/update_card.py -g MAN_WEB_BASE -k legacy.funcs.instanceCount -v exactly1 -y
}

function recluster() {
    echo "Adjusting and adding ints"
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_INT -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_CBIR_INT -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_PLATINUM_INT -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_TIER0_INT -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_PLATINUM_JUPITER_INT -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER0_JUPITER_INT -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INTL2 -c alloc_hosts,generate_intlookups

    # betas
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_CBIR_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_IMGS_CBIR_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_PLATINUM_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_PLATINUM_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_PLATINUM_JUPITER_BASE_MULTI -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_PLATINUM_JUPITER_INT_MULTI -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER0_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER0_JUPITER_BASE_MULTI -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_TIER0_JUPITER_INT_MULTI -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_PLATINUM_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_PLATINUM_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_TIER0_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_VIDEO_TIER0_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INTL2_MULTI -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g MAN_WEB_INTL2_HAMSTER -c alloc_hosts,generate_intlookups

    # psi yt
    YT_GROUPS=(
        MAN_YT_PROD_PORTOVM
        MAN_YT_TESTING0_PORTOVM
        MAN_YT_TESTING2_PORTOVM
        MAN_YT_TESTING1_PORTOVM
    )
    for GROUP in ${YT_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done
}

if [ "${1}" != "--source-only" ]; then
    select_action cleanup allocate_hosts recluster
fi
