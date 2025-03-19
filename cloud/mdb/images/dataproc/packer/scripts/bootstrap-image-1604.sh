#!/bin/bash
# Yandex Cloud Dataproc image.

set -x -euo pipefail
DAEMON_NAME=packer-dataproc-image
DATAPROC_REPO=${DATAPROC_REPO:-"https://dataproc.storage.yandexcloud.net/agent/"}
DATAPROC_AGENT_VERSION=${DATAPROC_AGENT_VERSION:-"latest"}

# Directory for temporary files
TMPDIR=$(mktemp -d)
BOOTDIR=/tmp/bootstrap
CA_PATH=/srv/CA.pem

log_fatal (){
    logger -ip daemon.error -t ${DAEMON_NAME} "$@"
    exit 1
}

log_info (){
    logger -ip daemon.info -t ${DAEMON_NAME} "$@"
}

cleanup () {
    rm -rf /var/log/yandex-dataproc-bootstrap.log
    truncate -s 0 /var/log/auth.log
    truncate -s 0 /var/log/dpkg.log
    truncate -s 0 /var/log/salt/minion
    truncate -s 0 /var/log/syslog
    truncate -s 0 /var/log/wtmp
    truncate -s 0 /var/log/yandex-dataproc-bootstrap.log
    rm -rf ${TMPDIR}
    rm -rf ${BOOTDIR}
    rm -rf /etc/network/interfaces.d/*
    rm -rf /var/lib/dhcp/*
}

setup_salt() {
    cp -r ${BOOTDIR}/pillar /srv/pillar
    cp -r ${BOOTDIR}/salt /srv/salt
    cp -r ${BOOTDIR}/dataproc.gpg /srv/dataproc.gpg
    cp -r ${BOOTDIR}/salt_minion /etc/salt/minion

    # Generate fake userdata for downloading deb packages for all dataproc components
    cat ${BOOTDIR}/vm-image-template.sls | \
        sed -e "s/vm-image-template.db.yandex.net/$(hostname -f)/g" \
        > /srv/pillar/userdata.sls
}

download_CA() {
    curl -fS --connect-time 3 --max-time 10 --retry 10 "https://storage.yandexcloud.net/cloud-certs/CA.pem" \
        -o ${CA_PATH} || log_fatal "Can't download Yandex Cloud CA"
}

setup_dataproc_diagnostics() {
    cp -r ${BOOTDIR}/dataproc-diagnostics.sh /usr/local/bin/dataproc-diagnostics.sh
    chmod 755 /usr/local/bin/dataproc-diagnostics.sh
}

setup_dataproc_agent() {
    DATAPROC_PATH="/opt/yandex/dataproc-agent"
    mkdir -p ${DATAPROC_PATH}/bin
    mkdir -p ${DATAPROC_PATH}/etc
    curl -fS --connect-time 3 --max-time 10 --retry 10 "${DATAPROC_REPO}dataproc-agent-${DATAPROC_AGENT_VERSION}" \
        -o ${DATAPROC_PATH}/bin/dataproc-agent || log_fatal "Can't download dataproc-agent"
    chmod 755 "${DATAPROC_PATH}/bin/dataproc-agent"
}

salt_highstate() {
    log_info "Running highstate"
    salt-call --state-out=changes \
        --output-diff \
        --local \
        --log-level=info \
        --retcode-passthrough \
        state.highstate queue=True || log_fatal "Failed to run highstate"

    /etc/init.d/salt-minion stop
    log_info "Removing builder minion_id"
    rm -rf /etc/salt/minion_id
}

setup_bootstrap_autoload() {
    mv "${BOOTDIR}/dataproc-start.sh" /srv/dataproc-start.sh
    # Add Data-Proc bootstrap script on every but when network is up and running
    mv "${BOOTDIR}/dataproc-bootstrap-service" /etc/init.d/dataproc-bootstrap
    chmod 755 /etc/init.d/dataproc-bootstrap
    update-rc.d dataproc-bootstrap defaults
}

uninstall_cloud_init() {
    DEBIAN_FRONTEND=noninteractive apt-get purge -y cloud-init || log_fatal "Failed uninstall cloud-init"
    rm -rf /etc/cloud || log_fatal "Failed remove /etc/cloud after for purgin cloud-init"
    userdel -f ubuntu || log_fatal "Failed to remove user ubuntu"
    rm -rf /var/log/cloud-init* || log_fatal "Failed to remove cloud-init logs"
    rm -f /etc/hostname
    truncate -s 0 /root/.ssh/authorized_keys || log_fatal "Failed to erase root authorized_keys"
}

hacks() {
    update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 || log_fatal "Failed to set locale"
    sed -i -e 's/#PermitRootLogin No/PermitRootLogin yes/g' /etc/ssh/sshd_config
}

main() {
    sleep 30 # This hack is needed for waiting unattended apt updates
    setup_salt
    download_CA
    setup_dataproc_diagnostics
    setup_dataproc_agent
    salt_highstate
    setup_bootstrap_autoload
    hacks
    uninstall_cloud_init
    cleanup
}

main
