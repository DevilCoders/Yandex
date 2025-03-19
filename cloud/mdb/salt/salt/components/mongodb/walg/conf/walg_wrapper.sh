#!/bin/bash
set -e

log () {
    echo -ne "$(date '+%F %T') [$$] $1: $2\n"
}
export -f log

export GODEBUG=madvdontneed=1

USAGE="Usage: $(basename $0) [-c <conf-dir>] [-t <timeout-minutes>] [-z <zk-lockfile>] [-w <zk-lock-wait-seconds>] [-s <sleep-seconds>] {backup_create [--permanent] [<backup_id>]| backup_delete <backup_name> |purge <before>|backup_restore <backup_name>|oplog_replay <from> <until>|oplog_purge}"

zk_lock_file=""
zk_lock_wait_seconds=5
sleep_seconds=""
timeout_minutes=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--confdir)
            conf_dir=${2:-default_confdir}
            shift 2;;
        -l|--lockfile)
            zk_lock_file="$2"
            shift 2;;
        -w|--lock_wait_seconds)
            zk_lock_wait_seconds="$2"
            shift 2;;
        -t|--timeout_minutes)
            timeout_minutes="$2"
            shift 2;;
        -s|--sleep_seconds)
            sleep_seconds="$2"
            shift 2;;
        -*)
            echo "$USAGE"; exit 1;;
        *)
            break
    esac
done


default_confdir=${conf_dir:-'/etc/wal-g'}
default_config='wal-g.yaml'
default_walg='wal-g-mongo'
default_cmd="${default_walg} --config ${default_confdir}/${default_config}"
export default_cmd



get_create_cmd () {
  cfg_dir=${1}
  backup_id=$2

  config="${cfg_dir}/${default_config}"
  if [ -n "${backup_id}" ]; then
    config="${cfg_dir}/wal-g-${backup_id}.yaml"
  fi

  echo "${default_walg} --config ${config}"
}


create_backup () {
    cmd=$1
    backup_id=$2

    if [ -n "${backup_id}" ]; then
      if ${default_cmd} backup-list -v | grep "${backup_id}"; then
	      log "backup with id '${backup_id}' already exists. No need to run backup creation. Exit now"
      	return 0
      fi
      log "backup with id '${backup_id}' was not found. Going to run backup creation"
    fi;
    ${cmd}
    exit_code=$?
    log "backup '${backup_id}' creation finished with exit code: ${exit_code}"
    return ${exit_code}
}
export -f create_backup

delete_backup () {
    cmd=$1
    backup_name=$2

    if ! (${default_cmd} backup-list -v | grep -q "${backup_name}") ; then
      log "backup with name '${backup_name}' was not found. No need to run backup deletion. Exit now"
      return 0
    fi
    log "backup '${backup_name}' was found. Going to run backup deletion"
    ${cmd}
}
export -f delete_backup


action=$1
case "$action" in
    backup_create)
        permanent=""
        backup_id=""

        if [ $# -eq 3 ]; then
          permanent=$2
          backup_id=$3
          shift 3
        elif [ $# -eq 2 ]; then
          if [ "$2" == "--permanent" ]; then
            permanent="--permanent"
          else
            backup_id=$2
          fi
          shift 2
        else
          shift 1
        fi

        cmd=$(get_create_cmd "${default_confdir}" "${backup_id}")

        cmd="create_backup '${cmd} backup-push ${permanent}' ${backup_id}"
        ;;

    backup_delete)
        if [ $# -ne 2 ]; then
          echo "${USAGE}"
          exit 1
        fi

        backup_name=$2
        cmd="delete_backup '${default_cmd} backup-delete ${backup_name} --confirm' ${backup_name}"

        shift 2;;
    purge)
        days=$2
        before=$(date --date="${days} days ago" "+%Y-%m-%dT%H:%M:%SZ")
        cmd="${default_cmd} delete --confirm  --retain-count ${days} --retain-after ${before} --purge-oplog"
        shift 2;;
    backup_restore)
        backup=$2
        cmd="${default_cmd} backup-fetch ${backup}"
        shift 2;;
    oplog_replay)
        from=$2
        until=$3
        cmd="${default_cmd} oplog-replay ${from} ${until}"
        shift 3;;
    backup_list)
        cmd="${default_cmd} backup-list"
        if [ $# -ge 1 ] && [ "$2" == "-v" ]; then
          cmd="${cmd} -v"
          shift 1
        fi
        shift 1;;
    oplog_purge)
        cmd="${default_cmd} oplog-purge --confirm"
        shift 1;;
    *)
        echo "$USAGE"
        exit 1
esac

if [ $# -ne 0 ] ; then
    echo "$USAGE"
    exit 1
fi


if [[ -n "$sleep_seconds" ]]; then
    cmd="sleep ${sleep_seconds} ; ${cmd}"
fi

if [[ -n "$zk_lock_file" ]]; then
    cmd="zk-flock -c ${zk_lock_file} lock -w ${zk_lock_wait_seconds} -x 3 \"bash -exc \\\"${cmd}\\\"\""
fi

if [[ -n "$timeout_minutes" ]]; then
    cmd="timeout ${timeout_minutes}m ${cmd}"
fi

log "$action" "Running:\n${cmd}"

bash -exc "${cmd}"
