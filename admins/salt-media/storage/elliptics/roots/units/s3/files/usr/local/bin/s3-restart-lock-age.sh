#!/usr/bin/env bash

lock_file="/var/tmp/s3-restart/lock"
max_lock_time=15

# Exits with zero when file was last modified more than <_time> minutes ago
function is_file_older_than() {
  local _path="${1}"
  local _time="${2}"

  ! find "${_path}" -mmin -"${_time}" | # prints only file with mtime less than '${_time}' minutes ago
    grep --quiet '' # sets exit code to 1 when no input given
}

# Exits with zero code when file is locked
function is_file_locked() {
  local _path="${1}"

  ! flock --exclusive --nonblock "${_path}" true
}

# Report status for monitoring and stop script execution
function report_status() {
  local _status="${1}"
  local _message="${2}"

  echo "${_status};${_message}"
  exit 0
}

if ! [ -e "${lock_file}" ]; then
  report_status "1" "s3-goose-restart lockfile does not exist"
fi

if is_file_older_than "${lock_file}" "${max_lock_time}" && is_file_locked "${lock_file}"; then
  report_status "2" "s3-goose-restart lockfile is locked too long"
fi

report_status "0" "Ok"
