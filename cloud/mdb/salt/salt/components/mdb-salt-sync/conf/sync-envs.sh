#!/bin/bash

set -e

/opt/yandex/mdb-salt-sync/fix-permissions.sh

sudo -u robot-pgaas-deploy /opt/yandex/mdb-salt-sync/mdb-salt-sync

/opt/yandex/mdb-salt-sync/fix-permissions.sh
