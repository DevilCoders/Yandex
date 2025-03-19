CREATE EXTENSION postgres_fdw;
CREATE SERVER postgresdb FOREIGN DATA WRAPPER postgres_fdw
    OPTIONS ( dbname 'postgres', host 'localhost');

CREATE USER MAPPING FOR monitor SERVER postgresdb
    OPTIONS (user 'admin', password '{{ salt['pillar.get']('data:config:pgusers:admin:password', '') }}');
ALTER USER MAPPING FOR monitor SERVER postgresdb
    OPTIONS (set user 'admin', set password '{{ salt['pillar.get']('data:config:pgusers:admin:password', '') }}');

CREATE FOREIGN TABLE IF NOT EXISTS public.monitor_stat_statements
(
    userid oid,
    dbid oid,
    queryid bigint,
    query text,
    calls bigint,
    total_time double precision,
    rows bigint,
    shared_blks_hit bigint,
    shared_blks_read bigint,
    shared_blks_dirtied bigint,
    shared_blks_written bigint,
    local_blks_hit bigint,
    local_blks_read bigint,
    local_blks_dirtied bigint,
    local_blks_written bigint,
    temp_blks_read bigint,
    temp_blks_written bigint,
    blk_read_time double precision,
    blk_write_time double precision
)
    SERVER postgresdb
        OPTIONS (table_name 'pg_stat_statements', schema_name 'public');

GRANT SELECT ON public.monitor_stat_statements TO monitor;
