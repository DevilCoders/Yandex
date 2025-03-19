#!/bin/bash
# Yandex Cloud Data Proc image 2.x

set -x -euo pipefail
DAEMON_NAME=packer-dataproc-image

# Directory for temporary files
TMPDIR=$(mktemp -d)
BOOTDIR=/tmp/bootstrap

log_fatal (){
    logger -ip daemon.error -t ${DAEMON_NAME} "$@"
    exit 1
}

log_info (){
    logger -ip daemon.info -t ${DAEMON_NAME} "$@"
}

cleanup () {
    rm -rf /var/log/yandex/*
    rm -rf /etc/salt/minion_id
    rm -rf /var/log/apt/*
    rm -rf /var/log/{auth,dpkg,syslog,wtmp,kern,alternatives,auth,btmp,dmesg}.log
    truncate -s 0 /var/log/salt/minion
    rm -rf /srv/pillar/vm-image-template.sls
    rm -rf /srv/pillar/ya.make
    rm -rf ${TMPDIR}
    rm -rf ${BOOTDIR}
    rm -rf /etc/network/interfaces.d/*
    rm -rf /var/lib/dhcp/*
    truncate -s 0 /home/ubuntu/.ssh/authorized_keys || log_fatal "Failed to clean authorized_keys"
    rm -rf /var/log/cloud-init* || log_fatal "Failed to remove cloud-init logs"
    rm -f /etc/hostname
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

setup_dataproc_diagnostics() {
    cp -r ${BOOTDIR}/dataproc-diagnostics.sh /usr/local/bin/dataproc-diagnostics.sh
    chmod 755 /usr/local/bin/dataproc-diagnostics.sh
}

setup_dataproc_agent() {
    DATAPROC_PATH="/opt/yandex/dataproc-agent"
    mkdir -p ${DATAPROC_PATH}/bin
    mkdir -p ${DATAPROC_PATH}/etc
    mkdir -p /var/log/yandex
}

setup_yc_cli() {
    curl https://storage.yandexcloud.net/yandexcloud-yc/install.sh | bash -s -- -i /opt/yandex-cloud -n
    echo "PATH=/opt/yandex-cloud/bin:\$PATH" >> /etc/profile.d/yc_cli.sh
}

salt_highstate() {
    log_info "Running highstate"
    salt-call --state-out=changes \
        --output-diff \
        --local \
        --log-level=info \
        --retcode-passthrough \
        state.highstate queue=True || log_fatal "Failed to run highstate"

    service salt-minion stop || log_fatal "Failed to stop minion"
    systemctl disable salt-minion || log_fatal "Failed to disable minion"
}

setup_bootstrap() {
    cp "${BOOTDIR}/dataproc-start.sh" /var/lib/cloud/scripts/per-boot/50-dataproc-start.sh
    mv "${BOOTDIR}/dataproc-init-actions.py" /var/lib/cloud/scripts/per-instance/50-dataproc-init-actions.py

    mv "${BOOTDIR}/dataproc-start-syslog.conf" /etc/rsyslog.d/80-yandex-dataproc-start.conf
}

hacks() {
    update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 || log_fatal "Failed to set locale"
    systemctl disable apt-daily
    systemctl disable apt-daily-upgrade
    sed -i -e 's/manage_etc_hosts: true/manage_etc_hosts: false/g' /etc/cloud/cloud.cfg.d/95-yandex-cloud.cfg || log_fatal 'Failed to disable /etc/hosts managing'
}

main() {
    setup_salt
    setup_dataproc_diagnostics
    setup_dataproc_agent
    setup_yc_cli
    salt_highstate
    setup_bootstrap
    hacks
    cleanup
}

main
