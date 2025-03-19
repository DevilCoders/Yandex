#!/bin/bash

cli="/opt/yandex/mdb-backup/bin/mdb-backup-cli/cli --config-path=/etc/yandex/mdb-backup/cli/"

log() {
    echo -ne "$(date '+%F %T') [$$] $1\n"
}
export -f log


switch_backup_service() {
   cid=$1
   enabled=$2
   run=$3
   cmd="${cli} backup_service_usage --enabled=${enabled} --cluster-id=${cid}"
   log "Command: ${cmd}"
   if [ "$run" != true ]; then
      log "Skipped due to dry run"
      return
   fi
   ${cmd}
}

roll_metadata() {
   cid=$1
   run=$2
   cmd="${cli} roll_metadata --cluster-id=${cid}"
   log "Command: ${cmd}"
   if [ "$run" != true ]; then
      log "Skipped due to dry run"
      return
   fi
   ${cmd}
}

import_backups() {
   cid=$1
   run=$2
   dryrun=true
   if [ "$run" == true ]; then
       dryrun=false
   fi
   cmd="${cli} import_backups --cluster-id=${cid} --dry-run=${dryrun} --skip-sched-date-dups"
   log "Command: ${cmd}"
   ${cmd}
}

USAGE="Usage: $(basename $0) [--yes] --cluster-id <cluster-id> --enabled (true|false)"

run=false
cid=""
enabled=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --yes)
            run=true
            shift 1;;
        --cluster-id)
            cid="$2"
            shift 1
	    shift 1;;
	--enabled)
	    enabled="$2"
	    shift 1
	    shift 1;;
        -*)
            echo "$USAGE"; exit 1;;
        *)
            break
    esac
done

if [ -z ${cid} ]; then
   echo "cluster-id expected"
   echo "$USAGE"
   exit 1;
fi

if [ "${enabled}" != "true" ] && [ "${enabled}" != "false" ]; then
   echo "bad enabled value: true or false expected"
   echo "$USAGE"
   exit 1;
fi


log "Switching backup-service for cluster ${cid}: status=${enabled}, real-run=${run}"
switch_backup_service "$cid" "$enabled" "$run"
if [ $? -ne 0 ]; then
    log "Failed. Exit now"
    exit 2
fi

log "Rolling metadata ${cid} on cluster ${cid} hosts"
roll_metadata "$cid" "$run"
if [ $? -ne 0 ]; then
    log "Failed. Exit now"
    exit 2
fi

if [ "${enabled}" == "true" ]; then
    log "Importing storage backups from cluster ${cid}"
    import_backups "$cid" "$run"
    if [ $? -ne 0 ]; then
        log "Failed. Exit now"
        exit 2
    fi
fi

log "Cluster ${cid} have completed successfully"
