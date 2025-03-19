#!/usr/bin/env bash

# https://wiki.yandex-team.ru/users/annkpx/naklikivanie-novyx-shardov-s3/#3.stavimduntajjm

cnt=0
for i in $(seq 64 64); do
  for c in "h" "k" "i"; do
    h=s3db$i$c.db.yandex.net
    echo "Host $h"
    set -x
#    # Step 3: Downtime
#    ssh -A jmon-master.search.yandex.net "jctl add downtime -host $h -stop +14days"
#
#    # Step 4: Deploy Group
#    mdb-admin -d porto-prod deploy minions upsert --group=porto-prod $h --autoreassign
#
#    # Wait
#
#    # Step 5.1
#    ssh root@"$h" "service mdb-ping-salt-master restart"
#    ssh root@"$h" "salt-call saltutil.sync_all & salt-call state.highstate queue=True" &
#
#    # Pause
#    # Step 5.2
#    ssh root@"$h" "salt-call state.highstate queue=True" &

#    # Check monitoring
#    ssh root@"$h" "sudo -u monitor monrun -w"

#    # After all: Downtime disable
#    ssh -A jmon-master.search.yandex.net "jctl remove downtime -host $h"

    set +x
    pids[$cnt]=$!
    hosts[$cnt]=$h
    ((cnt++))
  done
  sleep 1
done

# waiting
for i in "${!pids[@]}"; do
  host=${hosts[$i]}
  pid=${pids[$i]}
  echo "wait $pid"
  if wait $pid; then
    echo "Host $host success"
  else
    echo "Host $host failed"
  fi
done
