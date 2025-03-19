#!/usr/bin/env bash

set -e

################################################################################

YANDEX_CLOUD_DISK_AGENT_VERSION="198.tags.releases.nbs.stable-21-4-8"

YC_DISK_AGENT_SYSTEMD_VERSION="8835569.trunk.1129388867"

################################################################################

if ! which pssh > /dev/null; then
    echo "pssh not found"
fi

pssh run -q --max-failed 2 --no-yubikey --no-bastion "hostname" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo apt update -qq; sudo apt --allow-downgrades install -y yandex-cloud-blockstore-disk-agent=$YANDEX_CLOUD_DISK_AGENT_VERSION" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo apt --allow-downgrades install -y yc-nbs-disk-agent-systemd=$YC_DISK_AGENT_SYSTEMD_VERSION" L@./nodes.txt

pssh run -p8 --no-yubikey --no-bastion \
    "sudo systemctl enable blockstore-disk-agent && sudo systemctl stop blockstore-disk-agent && sudo systemctl start blockstore-disk-agent" L@./nodes.txt
