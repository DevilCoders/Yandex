#!/usr/bin/env bash
set -e

if [[ -z "$1" ]]; then
    echo "Please provide filepath to file that will store list of registries as the first argument"
    exit 1
fi
HOSTNAMES_FILE="$1"

ENDPOINT_FILE="/etc/yc/endpoint"
ENDPOINT="$(cat "$ENDPOINT_FILE")"

HOSTNAMES="cr.yandex,cr.cloud.yandex.net,container-registry.cloud.yandex.net"
if [[ "$ENDPOINT" == "api.cloud-preprod.yandex.net:443" ]]; then
        HOSTNAMES="cr.cloud-preprod.yandex.net,container-registry.cloud-preprod.yandex.net"
fi

echo "$HOSTNAMES" > "$HOSTNAMES_FILE"
