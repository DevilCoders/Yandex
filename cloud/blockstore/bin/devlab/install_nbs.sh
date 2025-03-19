#!/usr/bin/env bash

set -e

################################################################################

YANDEX_CLOUD_BLOCKSTORE_SERVER_VERSION="133.tags.releases.nbs.stable-21-2-5-8"
YC_NBS_SYSTEMD_VERSION="8319216.trunk.1000293674"

################################################################################

if ! which pssh > /dev/null; then
    echo "pssh not found"
fi

pssh run -q --max-failed 2 --no-yubikey --no-bastion "hostname" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo apt update -qq; sudo apt --allow-downgrades install -y yandex-cloud-blockstore-server=$YANDEX_CLOUD_BLOCKSTORE_SERVER_VERSION" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo chmod -R 777 /Berkanavt/" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo rm -rf /Berkanavt/nbs-server/cfg/" L@./nodes.txt

pssh scp -p8 --no-yubikey --no-bastion -R ./nbs-cfg L@./nodes.txt:/Berkanavt/nbs-server/

pssh run -p8 --no-yubikey --no-bastion \
    "sudo mv /Berkanavt/nbs-server/nbs-cfg/ /Berkanavt/nbs-server/cfg/" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo chown -R root:root /Berkanavt/" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo apt --allow-downgrades install -y yc-nbs-systemd=$YC_NBS_SYSTEMD_VERSION" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo systemctl enable nbs && sudo systemctl stop nbs && sudo systemctl start nbs" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo touch /Berkanavt/nbs-server/cfg/nbs_restart_on_deploy.conf" L@./nodes.txt
