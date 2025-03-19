SELECT rolname, queryid, round((total_time/calls)::numeric, 2) AS avg_time,
    pg_size_pretty(shared_blks_hit*8192/calls) AS shared_hit,
    pg_size_pretty(int8larger(0, (shared_blks_read*8192 - reads)/calls)) AS page_cache_hit,
    pg_size_pretty(reads/calls) AS physical_read,
    round((blk_read_time/calls)::numeric, 2) AS avg_read_time,
    round((user_time/calls)::numeric, 2) AS avg_user_time,
    round((system_time/calls)::numeric, 2) AS avg_system_time
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%'
ORDER BY reads/calls DESC LIMIT 10;
