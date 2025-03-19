#!/usr/bin/env bash

fix_perms() {
    chown -R robot-pgaas-deploy:dpt_virtual_robots_1561 "$1"
}

fix_perms /srv
fix_perms /salt-images

cd /salt-images && \
    sudo -u robot-pgaas-deploy /opt/yandex/mdb-salt-sync/mdb-salt-sync \
        --config-path=/etc/yandex/mdb-salt-sync \
        --upload
