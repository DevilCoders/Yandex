#!/bin/bash

log() {
    echo -ne "$(date '+%F %T') [$$] $1\n"
}
export -f log

if [ "$#" -lt 1 ] || [ "$#" -gt 3 ]; then
    echo "USAGE: $0 <comm> [delay seconds, default 15] [samples count, default 10]"
    exit 1
fi

comm=$1
delay=${2:-15}
samples=${3:-10}

short_host=$(hostname -s)
bt_dir="/var/tmp/${short_host}-bt-samples"
[ -d "$bt_dir" ] && echo "${bt_dir} already exists, exit now" && exit 1
mkdir -p "$bt_dir"

for i in $(seq 1 "$samples"); do
  pid=$(pidof "${comm}")
  [ -z "$pid" ] && echo "pid of ${comm} was not found, exit now" && exit 1
  log "[${i}/${samples}] Running gdb for pid ${pid} (comm ${comm})"
  if ! gdb -p $pid -batch -ex 'thread apply all bt' > "$bt_dir/stacks-$(date -u '+%Y-%m-%dT%H-%M-%S')Z.txt"; then
      log "gdb failed with error, exit now"
      exit 1
  fi
  log "[${i}/${samples}] Completed. Sleep for ${delay} seconds"
  sleep "$delay"
done

log "Samples dir: $bt_dir"
