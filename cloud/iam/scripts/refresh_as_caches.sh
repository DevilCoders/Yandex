#!/usr/bin/env bash
set -e
set -o pipefail

TICKET=${TICKET:?"TICKET env variable is required"}
ST_OAUTH_TOKEN=${ST_OAUTH_TOKEN:-$(cat ${ST_OAUTH_TOKEN_FILE:?"Either ST_OAUTH_TOKEN or ST_OAUTH_TOKEN_FILE env variable is required"})}
THIS_SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ARC_IAM_SCRIPTS=${THIS_SCRIPT_DIR?}
HOST_SPEC=${1:?}

pause=180
logfile=$(date +".refresh-as-cache-%Y%m%d%H%M%S.log")
source "$ARC_IAM_SCRIPTS/get_host_env.sh"

i=1
for host in $(pssh list -l $HOST_SPEC | sort); do
  echo "$i: $host"
  i=$((i + 1))
done
for host in $(pssh list -l $HOST_SPEC | sort); do
  this_host_env=$(get_host_env $host)
  if [ "$this_host_env" != "$host_env" ]; then
    host_env=$this_host_env
    my_token=$(ycp --profile $host_env iam create-token)
  fi
  for cache in $(echo "rolePermissions resourcePermissions"); do
    datetime=$(date +'%Y-%m-%dT%H:%M:%S%z')
    echo -n "$datetime $host $cache: " | tee -a $logfile
    grpcurl -H "Authorization: Bearer $my_token" -insecure \
      -d "{\"cache_name\": \"$cache\"}" \
      $host:4286 yandex.cloud.priv.iam.management.CacheService/Refresh \
      | jq -c '.' | tee -a $logfile
  done
  sleep 30
done

comment_log=$(cat $logfile | sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g') # uncolor
comment=$(printf 'refresh AS cache on %s: <{OK\n```\n%s\n```\n}>' $HOST_SPEC "$comment_log")
${ARC_IAM_SCRIPTS}/add_startrek_comment.sh "$TICKET" "$comment"
