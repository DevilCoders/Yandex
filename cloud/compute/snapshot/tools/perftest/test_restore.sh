#!/bin/bash

SNAPSHOT_CFG=/etc/yc/snapshot/config.toml
SNAPSHOT_ID=fdv1luq9fnm8emeq2l35
#DISK_ID=a7lt6i0ugt3qht0654s5
DISK_ID=null
CLUSTER_ID=ru-central1-a
#TASK_ID=test-vla
NBS_ENDPOINT=vla04-s15-10.cloud.yandex.net:9766
THREAD_COUNT=1
MOVE_WORKERS=$1
[ -z "$MOVE_WORKERS" ] && MOVE_WORKERS="1 2 4 8 16 32 64 128"

#set -x
set -e


for i in $MOVE_WORKERS; do
	echo "COUNT: $i"
        grep -q Performance "$SNAPSHOT_CFG" || echo '
[Performance]
MoveWorkers = 16' >> "$SNAPSHOT_CFG"
	sed -i "s/MoveWorkers.*/MoveWorkers = $i/g" "$SNAPSHOT_CFG"
	systemctl restart yc-snapshot
	sleep 1
        count=5
        [ $i -eq 1 -o $i -eq 2 ] && count=2
	PYTHONUNBUFFERED=1 ./test_restore.py \
		--task_id "$TASK_ID" \
		--snapshot_id "$SNAPSHOT_ID" \
		--cluster_id "$CLUSTER_ID" \
		--disk_id "$DISK_ID" \
		--nbs_endpoint "$NBS_ENDPOINT" \
		--count "$count" \
		--thread_count "$THREAD_COUNT"
	curl [::1]:7629/metrics 2>/dev/null | jq '
"lib_nbs_write_data_full_timer", .lib_nbs_write_data_full_timer, 
"lib_read_data_chunk_timer", .lib_read_data_chunk_timer, 
"lib_kikimr_get_snapshot_query_timer", .lib_kikimr_get_snapshot_query_timer, 
"lib_kikimr_read_chunk_query_timer", .lib_kikimr_read_chunk_query_timer, 
"lib_kikimr_commit_query_timer", .lib_kikimr_commit_query_timer,
"lib_kikimr_rollback_query_timer", .lib_kikimr_rollback_query_timer
'
done
