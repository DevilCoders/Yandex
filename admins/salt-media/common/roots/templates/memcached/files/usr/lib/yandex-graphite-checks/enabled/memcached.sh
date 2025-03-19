#!/bin/sh
{% set mport = salt['pillar.get']('memcached:port', 11211) -%}
# echo stats | nc -w 5 0 11211 | perl -nE'if(/^STAT\s+([\w_]+)\s+(\d+\.?\d*)\s*$/){$k=$1;$v=$2;if($k=~/^uptime|rusage_user|rusage_system|curr_connections|total_connections|cmd_get|cmd_set|cmd_flush|cmd_touch|get_hits|get_misses|delete_misses|delete_hits|incr_misses|incr_hits|decr_misses|decr_hits|cas_misses|cas_hits|cas_badval|touch_hits|touch_misses|bytes_read|bytes_written|limit_maxbytes|accepting_conns|hash_power_level|hash_bytes|bytes|curr_items|total_items|expired_unfetched|evicted_unfetched|evictions|reclaimed|crawler_reclaimed|crawler_items_checked$/){say"$k $v"}}'

echo stats | nc -w 5 0 {{mport}} | perl -nE'if(/^STAT\s+([\w_]+)\s+(\d+\.?\d*)\s*$/){$k=$1;$v=$2;say"$k $v"}'
