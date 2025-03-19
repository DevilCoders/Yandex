#!/bin/bash

set -e

/opt/yandex/mdb-salt-sync/sync-dev-and-private.sh
/opt/yandex/mdb-salt-sync/sync-envs.sh
