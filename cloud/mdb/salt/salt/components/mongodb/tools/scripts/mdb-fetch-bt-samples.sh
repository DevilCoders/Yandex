#!/bin/bash

set -e

ssh_cmd="pssh -uroot "
tmp_stdout_file="mdb-fetch-bt-samples.stdout.tmp"
tmp_stderr_file="mdb-fetch-bt-samples.stderr.tmp"

log() {
    echo -ne "$(date '+%F %T') [$$] $1\n"
}
export -f log

run_cmd() {
    host=$1
    cmd=$2
    (${ssh_cmd} "${host}" "${cmd}") 1>"${tmp_stdout_file}" 2>"${tmp_stderr_file}"
    if grep 'ERROR' "${tmp_stderr_file}"; then
        log "Data fetch failed with errors: $(cat ${tmp_stderr_file})"
        return 1
    fi
    if [ ! -s "${tmp_stdout_file}" ]; then
        log "Empty data file fetched: ${tmp_stdout_file}"
        return 1
    fi
}

if [ $# -ne 2 ]; then
    echo "${0} <hostname> <mongod|mongocfg|mongos>"
    exit 1
fi

host=$1
srv=$2

short_host=$(echo "$host" | awk -F'.' '{print $1}')
remote_bt_dir="/var/tmp/${short_host}-bt-samples"

arch="${short_host}_${srv}_backtrace_samples_$(date -u '+%Y-%m-%dT%H-%M-%S')Z.gz"

run_cmd "${host}" "test -e ${remote_bt_dir} && tar -czv ${remote_bt_dir} || ( export ret=\$?; [[ \$ret -eq 1 ]] || exit "\$ret" )" || exit $?
mv ${tmp_stdout_file} "${arch}"

rm -f "${tmp_stdout_file}"
rm -f "${tmp_stderr_file}"

log "Saved ${arch}"
