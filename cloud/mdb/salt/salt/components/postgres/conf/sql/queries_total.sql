SELECT rolname, queryid, round(total_time::numeric, 2) AS total_time, calls,
    pg_size_pretty((shared_blks_hit+shared_blks_read)*8192 - reads) AS memory_hit,
    pg_size_pretty(reads) AS disk_read,
    pg_size_pretty(writes) AS disk_write,
    round(blk_read_time::numeric, 2) AS blk_read_time,
    round(blk_write_time::numeric, 2) AS blk_write_time,
    round(user_time::numeric, 2) AS user_time,
    round(system_time::numeric, 2) AS system_time
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%'
ORDER BY total_time DESC LIMIT 10;
