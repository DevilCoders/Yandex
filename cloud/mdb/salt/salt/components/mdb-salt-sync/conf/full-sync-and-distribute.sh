#!/bin/bash

set -e

/opt/yandex/mdb-salt-sync/reset-srv.sh
/opt/yandex/mdb-salt-sync/sync-dev-and-private.sh
/opt/yandex/mdb-salt-sync/sync-envs.sh
/opt/yandex/mdb-salt-sync/distribute-sync.sh
