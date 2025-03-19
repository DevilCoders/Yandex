#!/usr/bin/env bash

set -o errexit
set -o nounset

: "Check tmp dir: ${CHECK_TMP_DIR:=/tmp/monrun_bad_ids_check/}"

function lock() {
    local _path="${CHECK_TMP_DIR}/lock"

    exec 99>"${_path}" # open FD to keep lock until the script end
    flock --exclusive 99

    # store current PID to lockfile
    echo "locked by '${0}' [${$}]" >&99
}

# Report status for monitoring and stop script execution
function report_status() {
  local _status="${1}"
  local _message="${2}"
  local _exitcode="${3:-0}"

  echo "${_status};${_message}"
  exit "${_exitcode}"
}

mkdir -p "${CHECK_TMP_DIR}"
# Lock the directory to the end of script to prevent concurrent execution races
# we create/modify/move files during the execution.
lock

bad_ids_file="${CHECK_TMP_DIR}bad_ids.txt"
err_log="${CHECK_TMP_DIR}err.log"

# Look for bad IDs on drives
timeout 1200 find /srv/storage/ -name ids -not -size 64c \
  1> "${bad_ids_file}" \
  2> "${err_log}" || true

# We expect 'lost+found' dirs to cause 'Permission denied' errors for 'find'
#  e.g.: find: ‘/srv/storage/44/lost+found/#404359854’: Permission denied
if grep -v 'lost+found' "${err_log}"; then
  # unexpected errors found here. Need to report them
  err_log_saved="${err_log}.saved"
  mv "${err_log}" "${err_log_saved}"

  report_status "2" "unexpected script error: $(tail -n 1 ${err_log_saved}). See more info at "${err_log_saved}""
fi

bad_ids_count="$(wc -l <"${bad_ids_file}")"

if [ "${bad_ids_count}" -gt 0 ]; then
  report_status "2" "'${bad_ids_count}' bad IDs detected"
fi

report_status "0" "Ok"
