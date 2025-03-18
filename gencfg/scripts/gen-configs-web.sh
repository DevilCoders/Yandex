#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 (build|clean)"
    exit 1
fi

GENERATED_DIR="./generated/intmetasearchv2"

run mkdir -p ${GENERATED_DIR}

if [ $1 == "build" ]; then
    CONFIGS=(
        arcnews/mmeta.yaml # arcnews
        fusion/mmeta.yaml # fusion
        imgs/mmeta.yaml # imgs mmeta
        imgs/int.yaml # imgs int
        imgsrq2/mmeta.yaml # imgsrq2
        musicyamrec/base.yaml # music yamrec base
        musicyamrec/mmeta.yaml # music yamrec mmeta
        musicyamrec/mmeta-test.yaml
        news/mmeta.yaml # news
        newsp/mmeta.yaml # newsp
        video/mmeta.yaml # video mmeta
        video/vhs_mmeta.yaml # vhs mmeta
        video/int.yaml # video int
        web/mmeta.yaml # web mmeta
    )

    MYARGS=()

    for CONFIG in ${CONFIGS[@]}; do
        MYARGS+=("./custom_generators/intmetasearchv2/generate_configs.py -a genconfigs -t db/configs/intmetasearchv2/${CONFIG} -o w-generated/all --workers 1")
    done
    MYARGS+=("./custom_generators/intmetasearchv2/generate_configs.py -a genconfigs -t db/configs/intmetasearchv2/web/int.yaml -o w-generated/all --workers 1 --extra-output-dir w-generated/web --anchors MAN_WEB_PLATINUM_BASE,MAN_WEB_PLATINUM_BASE_HAMSTER,MAN_WEB_PLATINUM_BASE_MULTI,MAN_WEB_TIER0_BASE,MAN_WEB_TIER0_BASE_HAMSTER,MAN_WEB_TIER0_BASE_MULTI,SAS_WEB_PLATINUM_BASE,SAS_WEB_PLATINUM_BASE_HAMSTER,SAS_WEB_TIER0_BASE,SAS_WEB_TIER1_BASE_MULTI,VLA_WEB_PLATINUM_BASE,VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER,VLA_WEB_PLATINUM_JUPITER_BASE_MULTI,VLA_WEB_TIER0_BASE,VLA_WEB_TIER0_BASE_HAMSTER,VLA_WEB_TIER0_BASE_MULTI")
    MYARGS+=("./custom_generators/intmetasearchv2/generate_configs.py -a genconfigs -t db/configs/intmetasearchv2/web/intl2.yaml -o w-generated/all --workers 1 --extra-output-dir w-generated/web --anchors MAN_WEB_TIER0_BASE,MAN_WEB_TIER0_BASE_HAMSTER,MAN_WEB_TIER0_BASE_MULTI,SAS_WEB_TIER0_BASE,SAS_WEB_TIER1_BASE_MULTI,VLA_WEB_TIER0_BASE,VLA_WEB_TIER0_BASE_HAMSTER,VLA_WEB_TIER0_BASE_MULTI")

    par "${MYARGS[@]}"

    status=$?
    if [ "${status}" -ne "0" ]; then
        exit ${status}
    fi
elif [ $1 == "clean" ]; then
    exit 0
    run ./utils/postgen/clean_generated_int_configs.py -g MSK_WEB_BASE,SAS_WEB_BASE,MAN_WEB_BASE,MAN_WEB_BASE_HAMSTER,MAN_WEB_BASE_PIP,SAS_WEB_BASE_HAMSTER,MAN_OXYGEN_BASE
else
    echo "Unknown action $1"
    exit 1
fi

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!!!!!!!!!!!!!!!!!!!! Everything is ok !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
