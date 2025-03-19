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

generate_nfs_packages() {
    environment=$1
    cluster=$2
    location=$3
    target=$4

    echo $environment-$cluster-$location $target

    CONFIG_DIR=kikimr/$environment/configs/yandex-cloud/$cluster/$location
    PACKAGE_DIR=kikimr/$environment/packages/yandex-cloud/$cluster/$location
    PACKAGE_NAME=yandex-search-kikimr-$target-conf-$cluster-$location

    mkdir -p $ARC_DIR/$PACKAGE_DIR/$target
    $BIN_DIR/generate-packages \
        --cfg-dir $CONFIG_DIR/$target/cfg \
        --pkg-dir $ARC_DIR/$PACKAGE_DIR/$target/$PACKAGE_NAME \
        --pkg-name $PACKAGE_NAME
}

generate_testing_packages() {
    generate_nfs_packages testing $@
}

generate_prod_packages() {
    generate_nfs_packages production $@
}

for cluster in "hw-nbs-dev-lab"; do
    generate_testing_packages $cluster global nfs
done

for cluster in "hw-nbs-stable-lab"; do
    for target in "nfs" "nfs-control"; do
        generate_testing_packages $cluster global $target
    done
done

for cluster in "testing"; do
    for target in "nfs" "nfs-control"; do
        generate_testing_packages $cluster vla $target
        generate_testing_packages $cluster myt $target
        generate_testing_packages $cluster sas $target
    done
done

for cluster in "preprod" "prod"; do
    for target in "nfs" "nfs-control"; do
        generate_prod_packages $cluster vla $target
        generate_prod_packages $cluster myt $target
        generate_prod_packages $cluster sas $target
    done
done

$BIN_DIR/config_generator -a $ARC_DIR -s $ARC_DIR/cloud/storage/core/tools/ops/config_generator/services/nfs
