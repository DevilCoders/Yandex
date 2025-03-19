#!/bin/bash -e

ARCADIA_ROOT="${ARCADIA_ROOT:-${HOME}/arcadia}"
SALT_ROOT="${ARCADIA_ROOT}/cloud/mdb/salt"
NS="salt-tremors"
REMOTE_SALT_ROOT="/srv/salt_images"
REMOTE_SALT_ROOT_FRONT="/srv/salt"

_exec_kube() {
    local pod=$1
    kubectl \
    --namespace=${NS} \
    exec --stdin \
    ${pod} \
    -- ${@:2}
}

_prepare_destination() {
    local pod=$1
    local dst_name=$2
    _exec_kube ${pod} mkdir -p "${dst_name}"
}

_transfer_package() {
    local pod=$1
    local dst=$2
    tar -cz -C ${SALT_ROOT} . | _exec_kube ${pod} tar -xz -C ${dst}
}

_finalize_package() {
    local pod=$1
    local actual_storage=$2
    old_link=$(_exec_kube ${pod} readlink ${REMOTE_SALT_ROOT_FRONT})
    _exec_kube ${pod} ln -fs ${actual_storage} ${REMOTE_SALT_ROOT_FRONT}
    test -z ${old_link} || _exec_kube ${pod} rm -fr ${old_link}
}

POD=$1
test -z ${POD} && { echo "usage: $0 <pod name>"; exit 1; }

dst_dir="${REMOTE_SALT_ROOT}/$(mktemp --dry-run -p '.' $(date +%s)-XXX)"
_prepare_destination ${POD} ${dst_dir}
_transfer_package ${POD} ${dst_dir}
_finalize_package ${POD} ${dst_dir}
