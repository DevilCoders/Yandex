SELECT rolname, queryid, round(total_time::numeric, 2) AS total_time, calls,
    pg_size_pretty(shared_blks_dirtied * 8192) AS shared_dirtied,
    pg_size_pretty(shared_blks_written * 8192) AS shared_written,
    pg_size_pretty(writes) AS disk_write,
    round(blk_write_time::numeric, 2) AS blk_write_time,
    round(user_time::numeric, 2) AS user_time,
    round(system_time::numeric, 2) AS system_time
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%'
ORDER BY writes DESC LIMIT 10;
