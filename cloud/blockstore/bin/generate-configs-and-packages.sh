#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

abspath() {
    path=$1

    if [ ! -d $path ]; then
        echo $path
        return
    fi

    (cd $1; pwd)
}

BIN_DIR=`find_bin_dir`
ARC_DIR=`abspath $BIN_DIR/../../../`

generate_prod_nbs_packages() {
    echo $1-$3 nbs

    CONFIG_DIR=kikimr/production/configs/yandex-cloud/$1/$3
    PACKAGE_DIR=kikimr/production/packages/yandex-cloud/$1/$3
    PACKAGE_NAME=yandex-search-kikimr-nbs-conf-$2-$3

    $BIN_DIR/generate-packages \
        --cfg-dir $CONFIG_DIR/nbs/cfg \
        --pkg-dir $ARC_DIR/$PACKAGE_DIR/nbs/$PACKAGE_NAME \
        --pkg-name $PACKAGE_NAME
}

generate_prod_nbs_control_packages() {
    echo $1-$3 nbs-control

    CONFIG_DIR=kikimr/production/configs/yandex-cloud/$1/$3
    PACKAGE_DIR=kikimr/production/packages/yandex-cloud/$1/$3
    PACKAGE_NAME=yandex-search-kikimr-nbs-control-conf-$2-$3

    if [ -d $ARC_DIR/$CONFIG_DIR/nbs_control/cfg/ ]; then
        $BIN_DIR/generate-packages \
            --cfg-dir $CONFIG_DIR/nbs_control/cfg \
            --pkg-dir $ARC_DIR/$PACKAGE_DIR/nbs_control/$PACKAGE_NAME \
            --pkg-name $PACKAGE_NAME
    fi
}

generate_prod_packages() {
    generate_prod_nbs_packages $@
    generate_prod_nbs_control_packages $@
}

generate_test_nbs_packages() {
    echo $1-$2 nbs

    CONFIG_DIR=kikimr/testing/configs/yandex-cloud/$1/$2
    PACKAGE_DIR=kikimr/testing/packages/yandex-cloud/$1/$2
    PACKAGE_NAME=yandex-search-kikimr-nbs-conf-$1-$2

    $BIN_DIR/generate-packages \
        --cfg-dir $CONFIG_DIR/nbs/cfg \
        --pkg-dir $ARC_DIR/$PACKAGE_DIR/nbs/$PACKAGE_NAME \
        --pkg-name $PACKAGE_NAME
}

generate_test_nbs_control_packages() {
    echo $1-$2 nbs-control

    CONFIG_DIR=kikimr/testing/configs/yandex-cloud/$1/$2
    PACKAGE_DIR=kikimr/testing/packages/yandex-cloud/$1/$2
    PACKAGE_NAME=yandex-search-kikimr-nbs-control-conf-$1-$2

    if [ -d $ARC_DIR/$CONFIG_DIR/nbs_control/cfg/ ]; then
        $BIN_DIR/generate-packages \
            --cfg-dir $CONFIG_DIR/nbs_control/cfg \
            --pkg-dir $ARC_DIR/$PACKAGE_DIR/nbs_control/$PACKAGE_NAME \
            --pkg-name $PACKAGE_NAME
    fi
}

generate_test_storage_configs() {
    echo $1-$2 storage

    CONFIG_DIR=kikimr/testing/configs/yandex-cloud/$1/$2

    $BIN_DIR/kikimr_configure cfg \
        --static \
        --grpc-endpoint grpc://localhost:2135 \
        $ARC_DIR/$CONFIG_DIR/config_cluster_template.yaml \
        $BIN_DIR/kikimr \
        $ARC_DIR/$CONFIG_DIR/storage/cfg/
}

generate_test_packages() {
    generate_test_nbs_packages $@
    generate_test_nbs_control_packages $@
    generate_test_storage_configs $@
}


for az in {myt,sas,vla}; do
    generate_prod_packages prod prod $az
    generate_prod_packages preprod pre-prod $az
done

for az in {myt,sas,vla}; do
    generate_test_packages testing $az
done

for cluster in {hw-nbs-dev-lab,hw-nbs-stable-lab}; do
    generate_test_packages $cluster global
done

$BIN_DIR/config_generator -a $ARC_DIR -s $ARC_DIR/cloud/storage/core/tools/ops/config_generator/services/nbs
