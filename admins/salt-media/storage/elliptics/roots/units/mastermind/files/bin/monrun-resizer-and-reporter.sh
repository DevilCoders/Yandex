#!/bin/bash

usage() {
cat<<HELP
SYNOPSIS
  $0  [ OPTIONS ]
OPTIONS
  -u,
      Ping URL. Default: ping
  -p,
      Ping PORT. Default: 80
  -t,
      Ping TIMEOUT. Default: 3
  -l,
      Log file for grep ERRORs
  -r,
      Monrun code for ERRRORs in log
  -h,
      Print a help message.

HELP
    exit 0
}


while getopts "hu:p:t:l:r:" opt; do
    case $opt in
        u)
            URL="$OPTARG"
            ;;
        p)
            PORT="$OPTARG"
            ;;
        t)
            TIMEOUT="$OPTARG"
            ;;
        l)
            LOG_FILE="$OPTARG"
            ;;
        r)
            LOG_REACTION="$OPTARG"
            ;;
        h)
            usage && exit 1
            ;;
        ?)
            usage && exit 1
            ;;
    esac
done

url=${URL:-"ping"}
port=${PORT:-80}
timeout=${TIMEOUT:-3}
log_file=${LOG_FILE:-''}
log_reaction=${LOG_REACTION:-2}

result=''
log_file=($(echo $log_file | tr ',' '\n'))


status=$(/usr/bin/jhttp.sh -u "/${url}" -p "${port}" -t "${timeout}")
if [ "${status}" != '0; ok' ]
then
    echo "${status}"
    exit 0
fi

timetail_time=3600

for log in "${log_file[@]}"
do
    errors=$(timetail -n "${timetail_time}" -t java "${log}" |grep ERROR | cut  -d ' ' -f 4- | wc -l)
    if [ "${errors}" -gt 0 ]
    then
        msg="${errors} errors in ${log}"
        result="${result};${msg}"
    fi
done

if [[ $result != '' ]]
then
    echo "${log_reaction}${result}"
else
    echo "0;OK"
fi
