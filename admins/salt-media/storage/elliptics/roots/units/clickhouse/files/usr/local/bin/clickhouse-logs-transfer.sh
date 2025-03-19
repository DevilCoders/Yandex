#!/bin/bash

LOG_TABLES=(
  avatars_access_logs
  cocaine_logs
  mds_access_logs_2
  s3_access_logs
  s3_cloud_access_logs
#  lepton_access_logs  # too few requests
#  avatars_test_access_logs  # too few requests
#  cocaine_test_logs  # too few requests
#  lepton_access_test_logs  # too few requests
#  logshatter  # too few requests
#  mds_test_access_logs  # too few requests
#  s3_test_access_logs  # too few requests
#  s3_cloud_preprod_access_logs  # too few requests
)

TIME_PERIOD=300
MIN_LINES=100

TIME_PERIOD_LR=3600
MIN_LINES_LR=5

PROBLEM_TABLES=""
ERRORS=""
RESULT=""

function check_last_record_age() {
  local _table="${1}"
  local _min_lines="${2}"
  local _time_period="${3}"

  local _count
  if ! _count=$(
    echo "SELECT count(*) FROM cocaine_logs.${_table} WHERE timestamp BETWEEN now()-${_time_period} AND now() ;" |
      clickhouse-client 2>/dev/null
  ); then
      ERRORS="${ERRORS} ${_table}"
      return
  fi

  if [ "${_count}" -ge "${_min_lines}" ]; then
    return
  fi

  local _age=0

  if [ "${_count}" -eq 0 ] ; then
      _age=$(
        echo "SELECT toInt32(now()-max(timestamp)) FROM cocaine_logs.${_table} ;" |
          clickhouse-client
      )
  else
      _age=$(
        echo "SELECT toInt32(now()-max(timestamp)) FROM cocaine_logs.${_table} WHERE timestamp BETWEEN now()-${_time_period} AND now() ;" |
          clickhouse-client
      )
  fi

  PROBLEM_TABLES="${PROBLEM_TABLES} ${_table} last record age ${_age}s;"
}

for table in "${LOG_TABLES[@]}" ; do
  check_last_record_age "${table}_lr" "${MIN_LINES_LR}" "${TIME_PERIOD_LR}"
  check_last_record_age "${table}" "${MIN_LINES}" "${TIME_PERIOD}"
done

if [ -z "${PROBLEM_TABLES}" ] && [ -z "${ERRORS}" ] ; then
    echo "0;OK"
    exit 0
fi

if [ -n "${PROBLEM_TABLES}" ] ; then
    RESULT=";Problems in tables:${PROBLEM_TABLES}"
fi

if [ -n "${ERRORS}" ] ; then
    RESULT="${RESULT};Errors in tables:${ERRORS}"
fi

echo "2${RESULT}"
