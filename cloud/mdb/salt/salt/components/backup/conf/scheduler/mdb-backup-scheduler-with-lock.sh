#!/usr/bin/env bash

ctype=$1
zk_flock_cfg=/etc/yandex/mdb-backup/scheduler/zk-flock-${ctype}.json
logs=/var/log/mdb-backup/
run_dir=/var/run/mdb-backup/scheduler/

if [ $# -ne 1 ] ; then
    echo "cluster_type expected (e.g., mongodb_cluster, postgresql_cluster, etc)"
    echo "$0 <cluster_type>"
    exit 1
fi

mkdir -p ${run_dir}
chown mdb-backup ${run_dir}
yazk-flock -c ${zk_flock_cfg} lock -x 42 "/opt/yandex/mdb-backup/bin/mdb-backup-scheduler/scheduler \
    --cluster-types="${ctype}" --schedule-create --schedule-obsolete --run-purge \
    --config-path=/etc/yandex/mdb-backup/scheduler/" >> ${logs}/scheduler-${ctype}.log 2>&1

echo "$?" > ${run_dir}/${ctype}-last-exit-status
