#!/bin/sh -e
{% set starting_statefile = salt.mdb_redis.get_redis_starting_flag() %}
/usr/local/yandex/redisctl/redisctl.py --action wait_started --restarts 1 || echo "wait start failed with $?"
/usr/local/yandex/redisctl/redisctl.py --action wait_replica_synced --restarts 1 || echo "wait replica sync failed with $?"
rm -rf {{ starting_statefile }} || echo "redis starting statefile drop failed with $?"
