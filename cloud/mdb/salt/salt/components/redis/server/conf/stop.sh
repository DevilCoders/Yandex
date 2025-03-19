#!/bin/sh -e
{% set stop_statefile = salt.mdb_redis.get_redis_server_stop_statefile() %}
{% set stopping_statefile = salt.mdb_redis.get_redis_stopping_flag() %}
touch {{ stopping_statefile }} || echo "redis stopping statefile set failed with $?"
/usr/local/yandex/ensure_no_primary.sh --skip-restart-check || echo "ensure-no-primary failed with $?"
/usr/local/yandex/redisctl/redisctl.py --action run_save_when_persistence_off || echo "save without persistence failed with $?"
/usr/local/yandex/redisctl/redisctl.py --action stop_and_wait --restarts 1 || echo "graceful stop failed with $?"
touch {{ stop_statefile }} || echo "correct stop statefile set failed with $?"
rm -rf {{ stopping_statefile }} || echo "redis stopping statefile drop failed with $?"
