#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/executor.sh
source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

function cleanup() {
    JUPITER_GROUPS=(
        VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER
        VLA_WEB_PLATINUM_JUPITER_INT_HAMSTER
        VLA_WEB_TIER0_JUPITER_BASE_HAMSTER
        VLA_WEB_TIER0_JUPITER_INT_HAMSTER
        VLA_WEB_INTL2_HAMSTER

        VLA_WEB_PLATINUM_JUPITER_BASE_MULTI
        VLA_WEB_PLATINUM_JUPITER_INT_MULTI
        VLA_WEB_TIER0_JUPITER_BASE_MULTI
        VLA_WEB_TIER0_JUPITER_INT_MULTI
        VLA_WEB_INTL2_MULTI
        
        VLA_WEB_PLATINUM_JUPITER_BASE
        VLA_WEB_PLATINUM_JUPITER_INT
        VLA_WEB_TIER0_JUPITER_BASE
        VLA_WEB_TIER0_JUPITER_INT
        VLA_WEB_INTL2
    )
    for GROUP in ${JUPITER_GROUPS[@]}; do
        # run ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
        run ./utils/common/update_igroups.py -a emptygroup -g ${GROUP}
    done

}

function allocate_hosts() {
    # VLA_YT_RTC arnold/fullhost
    
    echo "Generating intlookups"
    run ./optimizers/sas/main.py -g VLA_YT_RTC -t optimizers/sas/configs/arnold.yaml -o 18 -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) + len(filter(lambda x: x.tier_name == 'VideoPlatinum', shards)) <= 1" -e uniform --exclude-cpu-from VLA_WEB_CALLISTO_CAM_BASE,VLA_IMGS_BASE,VLA_IMGS_CBIR_BASE,VLA_VIDEO_TIER0_BASE,VLA_VIDEO_PLATINUM_BASE,VLA_IMGS_SAAS_QUICK_BASE,VLA_IMGS_PPL_BASE,VLA_YT_DATA_PROXIES,VLA_YT_NODES,VLA_YT_NODES_AMD,VLA_YT_NODES_NEW,VLA_YT_NODES_SSD_JOURNALS,VLA_YT_NODES_WITH_DATA_PROXIES,VLA_YT_RTC_NODES_WITHOUT_STORAGES --exclude-hosts-from VLA_PORTAL_ANY_PRESTABLE,VLA_PORTAL_MORDA,VLA_PORTAL_MORDA_PRESTABLE,VLA_PORTAL_MORDA_YARU_PRESTABLE,VLA_ADDRS_18SHARDS_BASE
    run ./utils/common/show_replicas_count.py -i VLA_YT_RTC.PlatinumTier0,VLA_YT_RTC.WebTier0
    run ./utils/common/fine_weights_tuning.py -s 30 -c optimizers/sas/configs/arnold.yaml
    run ./utils/postgen/adjust_replicas.py -a fix -c optimizers/sas/configs/arnold.yaml
    run ./utils/common/compare_sas_config_with_intlookups.py -c optimizers/sas/configs/arnold.yaml
}

function recluster() {
    run ./utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_YT_RTC.WebTier0
    run ./utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.PlatinumTier0 -g VLA_WEB_PLATINUM_JUPITER_BASE
    run ./utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.WebTier0 -g VLA_WEB_TIER0_JUPITER_BASE

    # VLA_YT_RTC.card exactly1/0, svn rm VLA_YT_RTC.instances
    
    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_PLATINUM_JUPITER_BASE -o VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER -I
    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_TIER0_JUPITER_BASE -o VLA_WEB_TIER0_JUPITER_BASE_HAMSTER -I
    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_TIER1_JUPITER_BASE -o VLA_WEB_TIER1_JUPITER_BASE_HAMSTER -I 

    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_PLATINUM_JUPITER_BASE -o VLA_WEB_PLATINUM_JUPITER_BASE_MULTI -I
    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_TIER0_JUPITER_BASE -o VLA_WEB_TIER0_JUPITER_BASE_MULTI -I
    run ./utils/postgen/shift_intlookup.py -i VLA_WEB_TIER1_JUPITER_BASE -o VLA_WEB_TIER1_JUPITER_BASE_MULTI -I 
}

select_action cleanup allocate_hosts recluster
