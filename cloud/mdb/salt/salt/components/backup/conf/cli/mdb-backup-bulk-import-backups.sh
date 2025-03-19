#!/usr/bin/env bash

ctype=$1
zk_flock_cfg=/etc/yandex/mdb-backup/cli/zk-flock-bulk-import-${ctype}.json
logs=/var/log/mdb-backup/
run_dir=/var/run/mdb-backup/cli/
batch_size=$2
interval=$3

if [ $# -ne 3 ] ; then
    echo "Usage: $(/usr/bin/basename "$0") <cluster-type> <batch-size> <interval>"
    exit 1
fi

mkdir -p ${run_dir}
chown mdb-backup ${run_dir}
yazk-flock -c ${zk_flock_cfg} lock -x 42 "/opt/yandex/mdb-backup/bin/mdb-backup-cli/cli import_backups \
    --cluster-types="${ctype}" --batch-size="${batch_size}" --interval="${interval}" --dry-run=false \
    --config-path=/etc/yandex/mdb-backup/cli/" \
    >> ${logs}/importer-${ctype}.log 2>&1

echo "$?" > ${run_dir}/${ctype}-last-exit-status
