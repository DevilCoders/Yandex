#!/bin/bash

switch_cmd="mdb-backup-switch-cluster.sh"

log() {
    echo -ne "$(date '+%F %T') [$$] >>>>>  $1\n"
}
export -f log


USAGE="Usage: $(basename "$0") [--yes] [--strict] --enabled (true|false)\nReads cluster ids from stdin."

run_flag=""
strict=false
completed=0
failed=0
enabled=""

ctype=mongodb

while [[ $# -gt 0 ]]; do
    case "$1" in
        --yes)
            run_flag="--yes"
            shift 1;;
        --strict)
            strict=true
            shift 1;;
        --enabled)
            enabled=$2
            shift 1
            shift 1;;
        --ctype)
            ctype=$2
            shift 1
            shift 1;;
        -*)
            echo -e "$USAGE"; exit 1;;
        *)
            break
    esac
done


if [ "${enabled}" != "true" ] && [ "${enabled}" != "false" ]; then
   echo "bad enabled value: true or false expected"
   echo -e "$USAGE"
   exit 1;
fi

while read -r cid ; do
    log "Running for cluster: ${cid}"
    full_cmd="${switch_cmd} ${run_flag} --cluster-id ${cid} --enabled ${enabled}"
    log "command: ${full_cmd}"
    ${full_cmd}
    if [ $? -ne 0 ]; then
       ((failed+=1))
       log "Cluster ${cid} failed"
       if [ "$strict" == true ]; then
           log " Exit now."
           break
       fi
    else
       log "Cluster ${cid} completed successfully"
       ((completed+=1))
    fi
    log "Backups page url: https://yc.yandex-team.ru/folders/mdb-junk/managed-${ctype}/cluster/${cid}?section=backups"
    echo -e "\n\n\n"
done

log "Completed: ${completed}, failed: ${failed}, total: $((completed + failed))"
