set -e

CONFIG="recluster2/126-vla/vla.yaml"

utils/common/update_card.py -g VLA_YT_RTC -k legacy.funcs.instanceCount -v vla_slot_count -y
utils/common/update_card.py -g VLA_YT_RTC -k legacy.funcs.instancePower -v fullhost -y

optimizers/sas/main.py -g VLA_YT_RTC -t $CONFIG \
    -o 18 \
    -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'PlatinumTier0', shards)) + len(filter(lambda x: x.tier_name == 'VideoPlatinum', shards)) <= 1" \
    -e uniform \
    --exclude-cpu-from VLA_WEB_CALLISTO_CAM_BASE,VLA_IMGS_SAAS_QUICK_BASE,VLA_IMGS_PPL_BASE,VLA_YT_DATA_PROXIES,VLA_YT_NODES,VLA_YT_NODES_AMD,VLA_YT_NODES_NEW,VLA_YT_NODES_SSD_JOURNALS,VLA_YT_NODES_WITH_DATA_PROXIES,VLA_YT_RTC_NODES_WITHOUT_STORAGES \
    --exclude-hosts-from VLA_PORTAL_ANY_PRESTABLE,VLA_PORTAL_MORDA,VLA_PORTAL_MORDA_PRESTABLE,VLA_PORTAL_MORDA_YARU_PRESTABLE,VLA_ADDRS_18SHARDS_BASE

utils/common/show_replicas_count.py -i VLA_YT_RTC.PlatinumTier0,VLA_YT_RTC.WebTier0,VLA_YT_RTC.VideoPlatinum,VLA_YT_RTC.VideoTier0,VLA_YT_RTC.ImgTier0
utils/common/fine_weights_tuning.py -s 30 -c $CONFIG
utils/postgen/adjust_replicas.py -a fix -c $CONFIG
utils/common/compare_sas_config_with_intlookups.py -c $CONFIG

utils/common/flat_intlookup.py -a unflat --process-intl2 -o 16 -i VLA_YT_RTC.WebTier0
utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.PlatinumTier0 -g VLA_WEB_PLATINUM_JUPITER_BASE
utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.WebTier0 -g VLA_WEB_TIER0_JUPITER_BASE
utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.ImgTier0 -g VLA_IMGS_BASE
utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.VideoPlatinum -g VLA_VIDEO_PLATINUM_BASE
utils/common/move_intlookup_to_slave.py -i VLA_YT_RTC.VideoTier0 -g VLA_VIDEO_TIER0_BASE

# don't ever do this until move_intlookups are done
utils/common/update_card.py -g VLA_YT_RTC -k legacy.funcs.instanceCount -v exactly1 -y
utils/common/update_card.py -g VLA_YT_RTC -k legacy.funcs.instancePower -v exactly0 -y
