CREATE SCHEMA IF NOT EXISTS plproxy;

-- handler function
CREATE OR REPLACE FUNCTION plproxy_call_handler ()
RETURNS language_handler AS 'plproxy' LANGUAGE C;

-- language
CREATE OR REPLACE LANGUAGE plproxy HANDLER plproxy_call_handler;

-- validator function
CREATE OR REPLACE FUNCTION plproxy_fdw_validator (text[], oid)
RETURNS boolean AS 'plproxy' LANGUAGE C;

-- foreign data wrapper
DROP FOREIGN DATA WRAPPER IF EXISTS plproxy CASCADE;
CREATE FOREIGN DATA WRAPPER plproxy VALIDATOR plproxy_fdw_validator;


CREATE OR REPLACE FUNCTION plproxy.get_cluster_version(i_cluster_name text)
RETURNS integer
LANGUAGE plpgsql AS $$
BEGIN
    IF i_cluster_name IN ('meta_rw', 'meta_ro', 'db_rw', 'db_ro') THEN
        RETURN 1;
    END IF;
    RAISE EXCEPTION 'Unknown cluster';
END;
$$;

CREATE OR REPLACE FUNCTION plproxy.get_cluster_config(
    i_cluster_name text,
    OUT o_key text,
    OUT o_val text
) RETURNS SETOF record
LANGUAGE plpgsql AS $$
BEGIN
    IF i_cluster_name NOT IN ('meta_rw', 'meta_ro', 'db_rw', 'db_ro') THEN
        RAISE EXCEPTION 'Unknown cluster';
    END IF;

    o_key := 'connection_lifetime';
    o_val := 1800::text;
    RETURN NEXT;
    RETURN;
END;
$$;

CREATE OR REPLACE FUNCTION plproxy.dynamic_query_meta(q text)
RETURNS SETOF RECORD AS $$
    CLUSTER 'meta_rw';
    RUN ON ALL;
    TARGET dynamic_query;
$$ LANGUAGE plproxy;

CREATE OR REPLACE FUNCTION plproxy.dynamic_query_db(q text)
RETURNS SETOF RECORD AS $$
    CLUSTER 'db_rw';
    RUN ON ALL;
    TARGET dynamic_query;
$$ LANGUAGE plproxy;

CREATE OR REPLACE FUNCTION plproxy.get_connstring(
    i_db_type text,
    i_shard_id int DEFAULT NULL
)
RETURNS SETOF text
LANGUAGE sql STABLE AS $function$
    SELECT * FROM plproxy.get_cluster_partitions(i_db_type)
    LIMIT CASE WHEN i_shard_id IS NULL THEN NULL ELSE 1 END
    OFFSET CASE WHEN i_shard_id IS NULL THEN 0 ELSE i_shard_id END
$function$;

DROP ROLE IF EXISTS s3api;
CREATE ROLE s3api;
DROP ROLE IF EXISTS s3api_ro;
CREATE ROLE s3api_ro;
DROP ROLE IF EXISTS s3api_list;
CREATE ROLE s3api_list;
