SELECT rolname, queryid, calls, round((total_time/calls)::numeric, 2) AS avg_time,
    pg_size_pretty(((shared_blks_hit+shared_blks_read)*8192 - reads)/calls) AS avg_memory_hit,
    pg_size_pretty(reads/calls) AS avg_disk_read,
    pg_size_pretty(writes/calls) AS avg_disk_write,
    round((blk_read_time/calls)::numeric, 2) AS avg_read_time,
    round((blk_write_time/calls)::numeric, 2) AS avg_write_time,
    round((user_time/calls)::numeric, 2) AS avg_user_time,
    round((system_time/calls)::numeric, 2) AS avg_system_time
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%'
ORDER BY avg_time DESC LIMIT 10;
