#!/bin/bash
# Script for installing base components for Yandex Cloud Search image.

set -x -euo pipefail
DAEMON_NAME=packer-yc-search-base

# Directory for temporary files
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
    truncate -s 0 /var/log/syslog
    truncate -s 0 /var/log/wtmp
    rm -rf /etc/network/interfaces.d/*
    rm -rf /var/lib/dhcp/*
}

clean_apt() {
    DEBIAN_FRONTEND=noninteractive apt-get clean || log_fatal "Can't clean apt"
}

download_CA() {
    curl -fS --connect-time 3 --max-time 10 --retry 10 "https://storage.yandexcloud.net/cloud-certs/CA.pem" \
        -o ${CA_PATH} || log_fatal "Can't download Yandex Cloud CA"
}


main() {
    sleep 5 # This hack is needed for waiting unattended apt updates
    download_CA
    clean_apt
    cleanup
}

main
