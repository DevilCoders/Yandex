#!/usr/bin/env bash

set -e

mkdir -p ./cfg/
mkdir -p ./nbs-cfg/

./kikimr_configure cfg --static ./config_cluster_template.yaml ./kikimr cfg/
./kikimr_configure cfg --dynamic ./config_cluster_template.yaml ./kikimr cfg/
./kikimr_configure cfg --nbs ./config_cluster_template.yaml /bin/true nbs-cfg/

echo "SuppressVersionCheck: true" >> ./cfg/names.txt

