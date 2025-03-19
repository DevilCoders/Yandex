#!/usr/bin/env bash

set -e

type=$1
container=$2

if [[ -z "$type" ]]; then
    echo "Container type is mandatory."
    exit 1
fi

if [[ -z "$container" ]]; then
    echo "Container name is mandatory."
    exit 1
fi

image_path="/data/images/${type}-template.tar.gz"

if [[ ! -f "${image_path}" ]]; then
    echo "${image_path} not found."
    exit 1
fi

path=`portoctl get ${container} root`

if [[ -d "${path}/bin" ]]; then
    echo "${path} is not empty."
    exit 1
fi

tar xf ${image_path} --numeric-owner -C ${path}

rsync -a /etc/yandex/selfdns-client/default.conf ${path}/etc/yandex/selfdns-client/default.conf
portoctl exec bootstrap-${container}-selfdns command="chown root:selfdns /etc/yandex/selfdns-client/default.conf" root="${path}"
hostname -f >${path}/etc/dom0hostname
echo ${container} >${path}/etc/hostname
