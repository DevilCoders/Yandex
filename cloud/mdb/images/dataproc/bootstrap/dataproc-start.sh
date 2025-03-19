#!/bin/bash
# Script for booting Yandex Cloud Dataproc

set -x -euo pipefail

# Script name for syslog
DAEMON_NAME=dataproc-start

# Directory for temporary files
TMPDIR=$(mktemp -d)

# Server for using user-data and meta-data
METASERVER=${METASERVER:-"http://169.254.169.254"}
# User-data from meta server
USERDATA=/srv/pillar/userdata.sls

FORCE=${FORCE:-"false"}

log_fatal (){
    logger -ip daemon.error -t ${DAEMON_NAME} "$@"
    cleanup
    exit 1
}

log_warn (){
    logger -ip daemon.warn -t ${DAEMON_NAME} "$@"
}

log_info (){
    logger -ip daemon.info -t ${DAEMON_NAME} "$@"
}

cleanup() {
    if [ -f "${TMPDIR}" ] ; then
        rm -rf "${TMPDIR}"
    fi
}

retry() {
    local max_attempts=${ATTEMPTS-8}
    local delay=${DELAY-1}
    local attempts=0
    local exitCode=0

    while [[ $attempts < $max_attempts ]]
    do
        "$@"
        exitCode=$?

        if [[ $exitCode == 0 ]]
        then
            break
        fi

        log_info "Failed attempts#${attempts} with rc ${exitCode}. Retrying in delay ${delay}"

        sleep "${delay}"
        attempts=$(( attempts + 1 ))
        delay=$(( delay * 2 ))
    done
    if [[ $exitCode != 0 ]]
    then
        log_warn "All attemps has failed with non-zero exit-code, last one: ${exitCode}"
    fi
    return $exitCode
}

download_userdata() {
    tmpfile="${TMPDIR}/user-data"
    retry curl -fS --connect-time 3 --max-time 5 --retry 5 -H Metadata-Flavor:Google \
        "${METASERVER}/computeMetadata/v1/instance/attributes/user-data" -o "${tmpfile}" \
        || log_warn "Can't download userdata"
    if [ -f "${tmpfile}" ] ; then
        grep -v -E "^#cloud-config$" "${tmpfile}" > "${USERDATA}"
    fi
}

setup_pillar() {
    log_info "Downloading userdata from metaserver"
    download_userdata || log_warn "Metaserver doesn't respond. Trying use existing ${USERDATA} on filesystem"
    if [ -f ${USERDATA} ] ; then
        log_info "Found local user-data, using it as a part of pillar"
        return
    fi
    log_fatal "Not found ${USERDATA} on filesystem"
}

salt_highstate() {
    # Do not run highstate if it disabled by flag and force is false
    if [[ "${FORCE}" == "false" ]] ; then
        local disable
        disable=$(salt-call --local pillar.get data:properties:dataproc:disable_highstate --no-color | grep True 2>&1 | sed -e 's/^[[:space:]]*//')
        if [[ "${disable}" == "True" ]] ; then
            log_info "Highstate disabled by special flag, skipping"
            return
        fi
    fi
    log_info "Clear salt cache"
    salt-call saltutil.clear_cache || log_fatal "Failed to clear salt caches"
    # Print grains before run highstate
    salt-call grains.items || log_fatal "Failed to print grains.items"
    log_info "Running highstate"
    salt-call --state-out=changes \
        --output-diff \
        --local \
        --log-level=info \
        --retcode-passthrough \
        state.highstate queue=True || log_fatal "Salt Highstate failed!"
}

extend_rootfs() {
    log_info "Extend rootfs to entire block device"
    if [ -b "/dev/vda" ] ; then
        /usr/bin/growpart /dev/vda 2 || log_info "Can't grow partition"
        resize2fs /dev/vda2 || log_warn "Can't resize ext4 partition"
    fi
}

ensure_hostname() {
    log_info "Ensuring correct hostname"
    # matters only for instance group clusters
    if [ -f /etc/hostname ]; then
        hostname "$(cat /etc/hostname)"
    fi
}

install() {
    extend_rootfs || log_fatal "Failed to extend fs"
    setup_pillar || log_fatal "Can't setting up pillar"
    salt_highstate || log_fatal "Failed salt highstate"
    ensure_hostname || log_fatal "Failed to set hostname"
    cleanup || log_fatal "Failed cleanup"
}

install
