SELECT rolname, queryid,
    100 * shared_hit / int8larger(1, shared_hit + shared_read) AS shared_buffer_hit,
    100 * int8larger(0, (shared_read - reads) / int8larger(1, shared_hit + shared_read)) AS system_cache_hit,
    100 * reads / int8larger(1, shared_hit + shared_read) AS physical_disk_read
FROM (
        SELECT userid, dbid, queryid,
            shared_blks_hit * 8192 AS shared_hit,
            shared_blks_read * 8192 AS shared_read
        FROM pg_stat_statements
    ) s
JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
JOIN pg_database d ON s.dbid = d.oid
JOIN pg_roles r ON r.oid = userid
ORDER BY physical_disk_read DESC
LIMIT 10;
