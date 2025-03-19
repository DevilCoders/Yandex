SELECT rolname, queryid, round((total_time/calls)::numeric, 2) AS avg_time, calls,
    pg_size_pretty(shared_blks_dirtied * 8192/calls) AS avg_shared_dirtied,
    pg_size_pretty(shared_blks_written * 8192/calls) AS avg_shared_written,
    pg_size_pretty(writes/calls) AS avg_disk_write,
    round((blk_write_time/calls)::numeric, 2) AS avg_write_time,
    round((user_time/calls)::numeric, 2) AS avg_user_time,
    round((system_time/calls)::numeric, 2) AS avg_system_time
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%'
ORDER BY writes/calls DESC LIMIT 10;
