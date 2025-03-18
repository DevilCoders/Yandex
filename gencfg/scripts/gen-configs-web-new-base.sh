#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

GENERATED_DIR="w-generated/all"
GENERATED_DIR_WEB="w-generated/web"
GENERATED_DIR_VIDEO="w-generated/video"

mkdir -p ${GENERATED_DIR} ${GENERATED_DIR_WEB} ${GENERATED_DIR_VIDEO}


function generate() {
    EMBEDDING_GROUP=$1
    INVERTED_INDEX_GROUP=$2
    CONFIG_TEMPLATE=$3

    if [ -n "$4" ]; then
        CONFIG_TEMPLATE="${CONFIG_TEMPLATE} --add-hamster"
    fi

    run ./custom_generators/new_base/embedding_configs.py \
        --target-dir ${GENERATED_DIR} --group ${EMBEDDING_GROUP} --inv-index-group ${INVERTED_INDEX_GROUP} \
        --config-template ${CONFIG_TEMPLATE}
}

function generate_yp() {
    EMBEDDING_GROUP=$1
    INVERTED_INDEX_YPLOOKUP=$2
    CONFIG_TEMPLATE=$3

    if [ -n "$4" ]; then
        CONFIG_TEMPLATE="${CONFIG_TEMPLATE} --workload $4"
    fi

    run ./custom_generators/new_base/embedding_configs_yp.py \
        --target-dir ${GENERATED_DIR} --group ${EMBEDDING_GROUP} --inv-index-yplookup ${INVERTED_INDEX_YPLOOKUP} \
        --config-template ${CONFIG_TEMPLATE}
}

function generate_yp2() {
    EMBEDDING_YPLOOKUP=$1
    INVERTED_INDEX_GROUP=$2
    CONFIG_TEMPLATE=$3

    if [ -n "$4" ]; then
        CONFIG_TEMPLATE="${CONFIG_TEMPLATE} --workload $4"
    fi

    run ./custom_generators/new_base/embedding_configs_yp2.py \
        --target-dir ${GENERATED_DIR_WEB} --yplookup ${EMBEDDING_YPLOOKUP} --inv-index-group ${INVERTED_INDEX_GROUP} \
        --config-template ${CONFIG_TEMPLATE}
}

function generate_yp3() {
    EMBEDDING_YPLOOKUP=$1
    CONFIG_TEMPLATE=$2
    OUTPUT_DIR=$3

    if [ -n "$4" ]; then
        CONFIG_TEMPLATE="${CONFIG_TEMPLATE} --workload $4"
    fi

    run ./custom_generators/new_base/embedding_configs_yp3.py \
        --target-dir ${OUTPUT_DIR} --yplookup ${EMBEDDING_YPLOOKUP} \
        --config-template ${CONFIG_TEMPLATE}
}

# web betas
generate VLA_WEB_TIER0_EMBEDDING_PIP VLA_WEB_TIER0_INVERTED_INDEX_PIP db/configs/new_base/web/embedding_tier0.proto.template
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX_MULTI db/configs/new_base/web/embedding_tier0.hamster.proto.template ${GENERATED_DIR_WEB} multi
# embedding betas: embedding@nanny -> invindex@deploy
generate_yp VLA_WEB_TIER0_EMBEDDING_BETA1 VLA_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.proto.template hamster
generate_yp VLA_WEB_TIER0_EMBEDDING_BETA2 VLA_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.proto.template hamster
generate_yp VLA_WEB_TIER0_EMBEDDING_BETA3 VLA_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.proto.template hamster
generate_yp VLA_WEB_TIER0_EMBEDDING_BETA4 VLA_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.proto.template hamster
generate_yp VLA_WEB_TIER0_EMBEDDING_BETA5 VLA_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.proto.template hamster

# web tier0 prod
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX db/configs/new_base/web/embedding_tier0.proto.template ${GENERATED_DIR_WEB} prod
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.hamster.proto.template ${GENERATED_DIR_WEB} hamster
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX_2 db/configs/new_base/web/embedding_tier0.proto.template ${GENERATED_DIR_WEB} prod
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX_HAMSTER_2 db/configs/new_base/web/embedding_tier0.hamster.proto.template ${GENERATED_DIR_WEB} hamster
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX_3 db/configs/new_base/web/embedding_tier0.proto.template ${GENERATED_DIR_WEB} prod
generate_yp3 SAS_WEB_TIER0_INVERTED_INDEX_HAMSTER_3 db/configs/new_base/web/embedding_tier0.hamster.proto.template ${GENERATED_DIR_WEB} hamster
generate_yp3 MAN_WEB_TIER0_INVERTED_INDEX db/configs/new_base/web/embedding_tier0.proto.template ${GENERATED_DIR_WEB} prod
generate_yp3 MAN_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.hamster.proto.template ${GENERATED_DIR_WEB} hamster
generate_yp3 VLA_WEB_TIER0_INVERTED_INDEX db/configs/new_base/web/embedding_tier0.proto.template ${GENERATED_DIR_WEB} prod
generate_yp3 VLA_WEB_TIER0_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier0.hamster.proto.template ${GENERATED_DIR_WEB} hamster

# imgs betas
generate VLA_IMGS_EMBEDDING_BETA VLA_IMGS_INVERTED_INDEX_BETA db/configs/new_base/web/embedding_tier1.proto.template
generate VLA_IMGS_EMBEDDING_PIP VLA_IMGS_INVERTED_INDEX_PIP db/configs/new_base/web/embedding_tier1.proto.template

# imgs prod
# generate VLA_IMGS_TIER0_EMBEDDING VLA_IMGS_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template YES
# generate MAN_IMGS_TIER0_EMBEDDING MAN_IMGS_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template YES
# imgs prod yp
generate_yp SAS_IMGS_TIER0_EMBEDDING SAS_IMGS_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template
generate_yp SAS_IMGS_TIER0_EMBEDDING SAS_IMGS_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier1.proto.template hamster
generate_yp MAN_IMGS_TIER0_EMBEDDING MAN_IMGS_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template
generate_yp MAN_IMGS_TIER0_EMBEDDING MAN_IMGS_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier1.proto.template hamster
generate_yp VLA_IMGS_TIER0_EMBEDDING VLA_IMGS_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template
generate_yp VLA_IMGS_TIER0_EMBEDDING VLA_IMGS_INVERTED_INDEX_HAMSTER db/configs/new_base/web/embedding_tier1.proto.template hamster

# video beta
generate VLA_VIDEO_PLATINUM_EMBEDDING_BETA VLA_VIDEO_PLATINUM_INVERTED_INDEX_BETA db/configs/new_base/web/embedding_tier1.proto.template
generate VLA_VIDEO_TIER0_EMBEDDING_BETA VLA_VIDEO_TIER0_INVERTED_INDEX_BETA db/configs/new_base/web/embedding_tier1.proto.template

# video prod @vla
generate_yp3 VLA_VIDEO_PLATINUM_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template ${GENERATED_DIR_VIDEO} prod
generate_yp3 VLA_VIDEO_TIER0_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template ${GENERATED_DIR_VIDEO} prod

# video prod @sas
generate_yp3 SAS_VIDEO_PLATINUM_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template ${GENERATED_DIR_VIDEO} prod
generate_yp3 SAS_VIDEO_TIER0_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template ${GENERATED_DIR_VIDEO} prod

# video prod @man
generate_yp3 MAN_VIDEO_PLATINUM_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template ${GENERATED_DIR_VIDEO} prod
generate_yp3 MAN_VIDEO_TIER0_INVERTED_INDEX db/configs/new_base/web/embedding_tier1.proto.template ${GENERATED_DIR_VIDEO} prod
