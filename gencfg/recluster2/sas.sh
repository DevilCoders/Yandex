#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    CLEANUP_GROUPS=(
        SAS_VIDEO_PLATINUM_BASE
        SAS_VIDEO_PLATINUM_INT
        SAS_VIDEO_PLATINUM_BASE_HAMSTER
        SAS_VIDEO_PLATINUM_INT_HAMSTER
        SAS_VIDEO_TIER0_BASE
        SAS_VIDEO_TIER0_INT
        SAS_VIDEO_TIER0_BASE_HAMSTER
        SAS_VIDEO_TIER0_INT_HAMSTER

        SAS_WEB_TIER0_JUPITER_BASE
        SAS_WEB_TIER0_JUPITER_INT
        SAS_WEB_TIER0_JUPITER_BASE_HAMSTER
        SAS_WEB_TIER0_JUPITER_INT_HAMSTER
        SAS_WEB_INTL2
        SAS_WEB_INTL2_HAMSTER

        SAS_IMGS_BASE
        SAS_IMGS_BASE_HAMSTER
        SAS_IMGS_BASE_NIDX
        SAS_IMGS_CBIR_BASE
        SAS_IMGS_CBIR_BASE_NIDX
        SAS_IMGS_CBIR_BASE_HAMSTER
        SAS_IMGS_CBIR_INT
        SAS_IMGS_CBIR_INT_HAMSTER
        SAS_IMGS_INT
        SAS_IMGS_INT_HAMSTER

        SAS_YT_PROD_PORTOVM
        SAS_YT_PROD2_PORTOVM
        SAS_YT_PROD3_PORTOVM
    )
    for GROUP in ${CLEANUP_GROUPS[@]}; do
        run ./utils/common/update_igroups.py -a emptygroup -g ${GROUP} 
    done

}

function allocate_hosts() {
# all hosts transfers must be done before this point
    run ./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instanceCount -v sas_web_video -y
    run ./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instancePower -v fullhost -y

    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g SAS_WEB_BASE -t optimizers/sas/configs/sas.web-video.yaml -o 18 \
        -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'VideoPlatinum', shards)) <= 1" \
        -e uniform \
        --exclude-cpu-from SAS_WEB_CALLISTO_CAM_BASE,SAS_IMGS_SAAS_QUICK_BASE,SAS_IMGS_PPL_BASE \
        --exclude-hosts-from SAS_PORTAL_ANY_STABLE,SAS_PORTAL_MORDA,SAS_PORTAL_MORDA_YARU_STABLE
    run ./utils/common/show_replicas_count.py -i SAS_WEB_BASE.WebTier0,SAS_WEB_BASE.VideoPlatinum,SAS_WEB_BASE.VideoTier0,SAS_WEB_BASE.ImgTier0
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/sas.web-video.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/sas.web-video.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/sas.web-video.yaml

    echo "Move intlookups to slave groups"
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.WebTier0 -g SAS_WEB_TIER0_JUPITER_BASE
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.VideoPlatinum -g SAS_VIDEO_PLATINUM_BASE
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.VideoTier0 -g SAS_VIDEO_TIER0_BASE
    run ./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.ImgTier0 -g SAS_IMGS_BASE
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_CBIR_BASE -c generate_intlookups 
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_BASE_NIDX -c generate_intlookups 
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_CBIR_BASE_NIDX -c generate_intlookups 

    echo "Cleanup master group"
    run ./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instanceCount -v exactly1 -y
    run ./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instancePower -v exactly0 -y
    run ./utils/common/manipulate_custom_instances_power.py -a remove -g SAS_WEB_BASE
}

function recluster() {
    echo "Adjusting and adding ints"
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_INT -c alloc_hosts,generate_intlookups 
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_CBIR_INT -c alloc_hosts,generate_intlookups 
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_PLATINUM_INT -c alloc_hosts,generate_intlookups 
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_TIER0_INT -c alloc_hosts,generate_intlookups 
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_INT -c alloc_hosts,generate_intlookups 
    run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_BASE.WebTier0 -o 16 --process-intl2
    # --- run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_TIER1_JUPITER_BASE -o 16 --process-intl2 
    # --- run ./utils/common/flat_intlookup.py -a unflat -i SAS_WEB_TIER1_JUPITER_BASE_HAMSTER -o 16 --process-intl2 
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_INTL2 -c alloc_hosts,generate_intlookups


    echo "Generate hamster intlookups"
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_TIER0_JUPITER_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_WEB_INTL2_HAMSTER -c alloc_hosts,generate_intlookups

    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_TIER0_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_TIER0_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_PLATINUM_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_VIDEO_PLATINUM_INT_HAMSTER -c alloc_hosts,generate_intlookups

    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_INT_HAMSTER -c alloc_hosts,generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_CBIR_BASE_HAMSTER -c generate_intlookups
    run ./tools/recluster/main.py -a recluster -g SAS_IMGS_CBIR_INT_HAMSTER -c alloc_hosts,generate_intlookups

    YT_GROUPS=(
        SAS_YT_PROD_PORTOVM
        SAS_YT_PROD2_PORTOVM
        SAS_YT_PROD3_PORTOVM
    )
    for GROUP in ${YT_GROUPS[@]}; do
        run ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts
    done
}

select_action cleanup allocate_hosts recluster
