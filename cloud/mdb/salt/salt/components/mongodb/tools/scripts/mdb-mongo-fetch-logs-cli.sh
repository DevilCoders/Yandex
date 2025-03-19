#!/bin/bash

set -u

yc_cmd="yc"
grep_cmd="/usr/local/bin/ggrep"
parallel=true
usage="${0} <cluster-id> <yc profile> <from> <until> <limit> [--sequent]"

log() {
    echo -ne "$(date '+%F %T') [$$] $1\n"
}
export -f log

fetch_for_host(){
    host=$1
    role=$2
    short_host=$(echo "$host" | awk -F'.' '{print $1}')

    case "$role" in
    "MONGOD")
        services=("mongod")
        ;;
    "MONGOINFRA")
        services=("mongocfg" "mongos")
        ;;
    "MONGOS")
        services=("mongos")
        ;;
    "MONGOCFG")
        services=("mongocfg")
        ;;
    *)
        log "Unknown mongodb host type"
        exit 1
        ;;
    esac
    for j in "${!services[@]}"; do
        srv=${services[$j]}

        log "$short_host $srv logs fetching"

        ${yc_cmd} --profile "$profile" managed-mongodb cluster list-logs --id "$cid" --since "${from}" --until "${until}" --limit "${limit}" --filter "message.hostname='${host}'" --service-type "${srv}" | ${grep_cmd} -oP '\K\{.*'  > "${short_host}.${srv}_${from}_${until}.log"
    done
}

if [ $# -lt 2 ]; then
    echo "$usage"
    exit 1
fi

cid=$1
profile=$2
from=$3
until=$4
limit=$5

shift 5;

while [[ $# -gt 0 ]]; do
case "$1" in
    --sequent)
	    parallel=false
	    shift 1
	    shift 1;;
    -*)
        echo "$usage"; exit 1;;
    *)
        break
    esac
done


log "Working on '${cid}', dbaas.py profile '${profile}'"

hosts=()
roles=()

while read -r line ; do
    hosts+=("$(echo "$line" | awk '{print $2}')")
    roles+=("$(echo "$line" | awk '{print $6}')")
done < <(yc --profile "$profile" managed-mongodb hosts list  --cluster-id "$cid" --format text  | grep 'mdb.yandexcloud.net')
echo "${hosts[@]}"

log "${#hosts[@]} hosts found: ${hosts[*]}"

for i in "${!hosts[@]}"; do
  log_prefix="[$((i + 1))/${#hosts[@]}]"
  host=${hosts[$i]}
  role=${roles[$i]}

  log "${log_prefix} Starting host ${host} (${role})"
  if [ "${parallel}" = true ]; then
      fetch_for_host "$host" "$role" &
      pids+=($!)
  else
      fetch_for_host "$host" "$role" || exit 1
      log "${log_prefix} Finished host ${host} (${role})"
  fi
done

exit_code=0
for i in "${!pids[@]}"; do
    log_prefix="[$((i + 1))/${#hosts[@]}]"
    pid=${pids[$i]}
    host=${hosts[$i]}
    role=${roles[$i]}

    wait "$pid"
    pid_exit_code=$?
    if [ "$pid_exit_code" -eq 0 ]; then
        log "${log_prefix} Finished host ${host} (${role})"
    else
        log "${log_prefix} FAILED host ${host} (${role}) with code: $pid_exit_code"
        exit_code=$pid_exit_code
    fi
done

exit $exit_code
