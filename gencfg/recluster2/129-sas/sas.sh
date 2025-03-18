CFG=sas.yaml
EXCLUDE_CPU=SAS_WEB_CALLISTO_CAM_BASE,SAS_IMGS_SAAS_QUICK_BASE,SAS_IMGS_PPL_BASE,SAS_WEB_TIER1_JUPITER_BASE,SAS_WEB_TIER1_EMBEDDING,SAS_WEB_TIER1_JUPITER_INT

./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instanceCount -v sas_slot_count -y
./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instancePower -v fullhost -y

echo "Generating intlookups"
./optimizers/sas/main.py -g SAS_WEB_BASE -t $CFG -o 18 \
    -l "lambda hgroup, shards: len(filter(lambda x: x.tier_name == 'VideoPlatinum', shards)) <= 1" \
    -e uniform \
    --exclude-cpu-from $EXCLUDE_CPU \
    --exclude-hosts-from SAS_PORTAL_ANY_STABLE,SAS_PORTAL_MORDA,SAS_PORTAL_MORDA_YARU_STABLE

./utils/common/show_replicas_count.py -i SAS_WEB_BASE.VideoPlatinum,SAS_WEB_BASE.VideoTier0,SAS_WEB_BASE.ImgTier0
./utils/common/fine_weights_tuning.py -s 30 -c $CFG
./utils/postgen/adjust_replicas.py -a fix -c $CFG
./utils/common/compare_sas_config_with_intlookups.py -c $CFG


echo "Move intlookups to slave groups"
./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.VideoPlatinum -g SAS_VIDEO_PLATINUM_BASE
./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.VideoTier0 -g SAS_VIDEO_TIER0_BASE
./utils/common/move_intlookup_to_slave.py -i SAS_WEB_BASE.ImgTier0 -g SAS_IMGS_BASE
./tools/recluster/main.py -a recluster -g SAS_IMGS_CBIR_BASE -c generate_intlookups 

echo "Cleanup master group"
./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instanceCount -v exactly1 -y
./utils/common/update_card.py -g SAS_WEB_BASE -k legacy.funcs.instancePower -v exactly0 -y
./utils/common/manipulate_custom_instances_power.py -a remove -g SAS_WEB_BASE
