SELECT rolname, queryid, round(total_time::numeric, 2) AS total_time, calls,
    pg_size_pretty(shared_blks_hit*8192) AS shared_hit,
    pg_size_pretty(int8larger(0, (shared_blks_read*8192 - reads))) AS page_cache_hit,
    pg_size_pretty(reads) AS physical_read,
    round(blk_read_time::numeric, 2) AS blk_read_time,
    round(user_time::numeric, 2) AS user_time,
    round(system_time::numeric, 2) AS system_time
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%'
ORDER BY reads DESC LIMIT 10;
