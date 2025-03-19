#!/bin/bash
set -e

cd $(dirname $0)
for conf in solomon/*/config.yaml; do
    if [[ "$conf" != *channels* && "$conf" != *base* ]]; then  # We skip 'channels' dir, because it is not per-team dir.
        yc-solomon-cli check-syntax -c "$conf"
    fi
done

cd juggler
for env in testing preprod prod israel dev-builds; do
    ./sync-checks.sh $env
done
./sync-dashboards.sh
