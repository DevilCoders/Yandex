#!/usr/bin/env bash
TICKET=${TICKET:?"TICKET env variable is required"}
THIS_SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ARC_IAM_SCRIPTS=${THIS_SCRIPT_DIR?}
source ${ARC_IAM_SCRIPTS}/juggler_downtimes.sh
HOST_SPEC=${1:?}

set -e
set -o pipefail

pause=180
logfile=$(date +".restart-as-%Y%m%d%H%M%S.log")
echo "Restarting following AS instances sequentially with $pause seconds pause, logfile '$logfile'"
restart_cmd="date -uIs && sudo service yc-access-service restart && date -uIs && sleep $pause && date -uIs && sudo monrun -r access-service"

# `| sort` - for predictable order with xDS. Fixed in CLOUD-104361 but it will take time to update pssh for everyone.
i=1
for host in $(pssh list -l $HOST_SPEC | sort); do
  echo "$i: $host"
  i=$((i + 1))
done
for host in $(pssh list -l $HOST_SPEC | sort); do
  add_downtime $host 30
  pssh run -p 1 "$restart_cmd" $host | tee -a $logfile
  remove_downtime
  add_downtime $host 1 # unlucky timing may cause an alert if the downtime is removed too early, so keep it for another minute
done

comment_log=$(cat $logfile | sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g') # uncolor
comment=$(printf 'restart AS on %s: <{OK\n```\n%s\n```\n}>' $HOST_SPEC "$comment_log")
${ARC_IAM_SCRIPTS}/add_startrek_comment.sh "$TICKET" "$comment"
