#!/bin/sh -e
{% set starting_statefile = salt.mdb_redis.get_redis_starting_flag() %}
{% set stop_statefile = salt.mdb_redis.get_redis_server_stop_statefile() %}
touch {{ starting_statefile }} || echo "redis starting statefile set failed with $?"
[ -f "{{ stop_statefile }}" ] || /usr/local/yandex/redisctl/redisctl.py --action crash_prestart_when_persistence_off || echo "crash prestart failed with $?"
rm -rf {{ stop_statefile }} || echo "correct stop statefile remove failed with $?"
su redis -c /usr/local/yandex/redis_fix_ip.py && echo "fix ip ok" || echo "fix ip failed with $?"
su redis -c /usr/local/yandex/redis_fix_aof.sh && /bin/true
