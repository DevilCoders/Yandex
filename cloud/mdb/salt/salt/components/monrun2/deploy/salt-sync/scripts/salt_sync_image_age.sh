#!/usr/bin/env bash

cd /etc/yandex/mdb-salt-sync && sudo -u sudo -u robot-pgaas-deploy \
    /opt/yandex/mdb-salt-sync/mdb-salt-image-age $*
