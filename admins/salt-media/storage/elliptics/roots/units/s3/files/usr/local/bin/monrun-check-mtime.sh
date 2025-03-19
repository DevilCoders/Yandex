#!/usr/bin/env bash

set -o errexit
set -o nounset

# Convert minutes to seconds and back (with loss of fraction)
function min_to_sec() { bc <<<"${1} * 60"; }
function sec_to_min() { bc <<<"${1} / 60"; }

file_path="${1}"
warn_age="$(min_to_sec "${2}")" # we get time in minutes from caller
crit_age="$(min_to_sec "${3}")" # we get time in minutes from caller
absent_status="${4:-0}"

# Get current file age in seconds
function file_age_sec() {
  local _path="${1}"

  local _now
  local _mtime

  _now="$(date '+%s')"
  _mtime="$(stat -c %Y "${_path}")"

  echo "${_now} - ${_mtime}" | bc || true
}

# Report status for monitoring and stop script execution
function report_status() {
  local _status="${1}"
  local _message="${2}"

  echo "${_status};${_message}"
  exit 0
}

if ! [ -e "${file_path}" ]; then
  report_status "${absent_status}" "'${file_path}' does not exist"
fi

age="$(file_age_sec "${file_path}")"

if [ "${age}" -gt "${crit_age}" ]; then
  report_status "2" "'${file_path}' last update was more than '$(sec_to_min "${age}")' minutes ago"
fi

if [ "${age}" -gt "${warn_age}" ]; then
  report_status "1" "'${file_path}' last update was more than '$(sec_to_min "${age}")' minutes ago"
fi

report_status "0" "Ok"
