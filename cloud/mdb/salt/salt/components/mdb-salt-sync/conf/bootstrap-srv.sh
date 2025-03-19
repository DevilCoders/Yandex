#!/bin/bash

set -e

# TODO: either move to mdb-salt-sync or use repos.sls

if ! svn info /srv; then
    sudo -u robot-pgaas-deploy svn checkout svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/cloud/mdb/salt /srv
    /opt/yandex/mdb-salt-sync/fix-permissions.sh
fi

if ! git -C /srv/pillar/private rev-parse HEAD; then
    sudo -u robot-pgaas-deploy git clone git@github.yandex-team.ru:mdb/salt-private.git /srv/pillar/private
    /opt/yandex/mdb-salt-sync/fix-permissions.sh
fi

/opt/yandex/mdb-salt-sync/sync-envs.sh
