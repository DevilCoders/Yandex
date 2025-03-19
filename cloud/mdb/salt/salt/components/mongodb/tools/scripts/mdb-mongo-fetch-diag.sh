#!/bin/bash

set -u

ssh_cmd="pssh -u root"
dbaaspy_cmd="dbaas"
parallel=true
usage="${0} <cluster-id> <dbaas.py profile> [--sequent]"

log() {
    echo -ne "$(date '+%F %T') [$$] $1\n"
}
export -f log


run_cmd() {
    host=$1
    cmd=$2
    stdout_file=$3
    stderr_file=$4
    (${ssh_cmd} "${host}" "${cmd}") 1>"${stdout_file}" 2>"${stderr_file}"
    if grep 'ERROR' "${stderr_file}"; then
        log "Data fetch failed with errors: $(cat "${stderr_file}")"
        return 1
    fi
    if [ ! -s "${stdout_file}" ]; then
        log "Empty data file fetched: ${stdout_file}"
        return 1
    fi
}

fetch_from_host(){
    host=$1
    role=$2
    short_host=$(echo "$host" | awk -F'.' '{print $1}')

    case "$role" in
    "{mongodb_cluster.mongod}")
        services=("mongod")
        logs=("/var/log/mongodb/mongodb.log*")
        diags=("/var/lib/mongodb/diagnostic.data")

        ;;
    "{mongodb_cluster.mongoinfra}")
        services=("mongocfg" "mongos")
        logs=("/var/log/mongodb/mongocfg.log*" "/var/log/mongodb/mongos.log*")
        diags=("/var/lib/mongodb/diagnostic.data" "/var/log/mongodb/mongos.diagnostic.data")
        ;;
    "{mongodb_cluster.mongos}")
        services=("mongos")
        logs=("/var/log/mongodb/mongos.log*")
        diags=("/var/log/mongodb/mongos.diagnostic.data")
        ;;
    "{mongodb_cluster.mongocfg}")
        services=("mongocfg")
        logs=("/var/log/mongodb/mongocfg.log*")
        diags=("/var/lib/mongodb/diagnostic.data")
        ;;
    *)
        log "Unknown mongodb host type"
        exit 1
        ;;
    esac
    for j in "${!services[@]}"; do
        srv=${services[$j]}
        log=${logs[$j]}
        diag=${diags[$j]}

        log "$short_host $srv logs fetching from ${log}"
        tmp_stdout_file="mdb_fetch_diag.${short_host}.stdout.tmp"
        tmp_stderr_file="mdb_fetch_diag.${short_host}.stderr.tmp"
        run_cmd "${host}" "tar -czv ${log} || ( export ret=\$?; [[ \$ret -eq 1 ]] || exit \"\$ret\" )" "${tmp_stdout_file}" "${tmp_stderr_file}" || return $?
        mv "${tmp_stdout_file}" "${short_host}_${srv}_logs_$(date -u '+%Y-%m-%dT%H-%M-%S')Z.tar.gz"
        log "$short_host $srv logs completed"

        log "$short_host $srv diagnostic.data fetching from ${diag}"
        run_cmd "${host}" "tar -czv ${diag} || ( export ret=\$?; [[ \$ret -eq 1 ]] || exit \"\$ret\" )" "${tmp_stdout_file}" "${tmp_stderr_file}" || return $?
        mv "${tmp_stdout_file}" "${short_host}_${srv}_diagnostic.data_$(date -u '+%Y-%m-%dT%H-%M-%S')Z.tar.gz"
        log "$short_host $srv diagnostic.data completed"

        rm "${tmp_stderr_file}"
    done
}

if [ $# -lt 2 ]; then
    echo "$usage"
    exit 1
fi

cid=$1
profile=$2
shift 2;

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
    hosts+=("$(echo "$line" | awk '{print $1}')")
    roles+=("$(echo "$line" | awk '{print $3}')")
done < <(${dbaaspy_cmd} --profile "$profile" host list -c "$cid" | grep 'db.yandex.net')
echo "${hosts[@]}"

log "${#hosts[@]} hosts found: ${hosts[*]}"
pids=()

for i in "${!hosts[@]}"; do
  log_prefix="[$((i + 1))/${#hosts[@]}]"
  host=${hosts[$i]}
  role=${roles[$i]}

  log "${log_prefix} Starting host ${host} (${role})"
  if [ "${parallel}" = true ]; then
      fetch_from_host "$host" "$role" &
      pids+=($!)
  else
      fetch_from_host "$host" "$role" || exit 1
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
