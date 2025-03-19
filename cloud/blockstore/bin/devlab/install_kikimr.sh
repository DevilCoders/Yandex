#!/usr/bin/env bash

set -e

################################################################################

YANDEX_SEARCH_KIKIMR_KIKIMR_BIN_VERSION="137.tags.releases.ydb.stable-21-2-24.hardening"
YC_KIKIMR_SYSTEMD_VERSION="7620865.trunk.26"

################################################################################

if ! which pssh > /dev/null; then
    echo "pssh not found"
fi

pssh run -q --max-failed 2 --no-yubikey --no-bastion "hostname" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo apt update -qq; sudo apt --allow-downgrades install -y yandex-search-kikimr-kikimr-bin=$YANDEX_SEARCH_KIKIMR_KIKIMR_BIN_VERSION" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo chmod -R 777 /Berkanavt/" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo rm -rf /Berkanavt/kikimr/cfg/" L@./nodes.txt

pssh scp -p8 --no-yubikey --no-bastion -R ./cfg L@./nodes.txt:/Berkanavt/kikimr/

pssh run -p8 --no-yubikey --no-bastion \
    "sudo chown -R root:root /Berkanavt/" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo apt --allow-downgrades install -y yc-kikimr-systemd=$YC_KIKIMR_SYSTEMD_VERSION" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo systemctl enable kikimr && sudo systemctl start kikimr && sudo systemctl stop kikimr" L@./nodes.txt

sleep 10

pssh run -p8 --no-yubikey --no-bastion \
    "sudo dd if=/dev/zero of=/dev/disk/by-partlabel/NVMEKIKIMR01 bs=1M count=1" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo dd if=/dev/zero of=/dev/disk/by-partlabel/NVMEKIKIMR02 bs=1M count=1" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo dd if=/dev/zero of=/dev/disk/by-partlabel/NVMEKIKIMR03 bs=1M count=1" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo dd if=/dev/zero of=/dev/disk/by-partlabel/ROTKIKIMR01 bs=1M count=1" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo systemctl start kikimr" L@./nodes.txt

sleep 10

pssh run --no-yubikey --no-bastion \
    "cd /Berkanavt/kikimr/ && ln -fs /usr/bin/kikimr && bash ./cfg/init_storage.bash  &&  bash ./cfg/init_compute.bash && bash ./cfg/init_databases.bash &&  bash ./cfg/init_root_storage.bash && bash ./cfg/init_cms.bash" L@./nodes.txt[0]

