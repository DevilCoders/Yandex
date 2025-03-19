CREATE TABLE IF NOT EXISTS plproxy.part_db_type (
    part_id integer,
    db_type text
);

CREATE TABLE IF NOT EXISTS plproxy.my_dc (
    dc text
);


CREATE OR REPLACE FUNCTION plproxy.get_cluster_partitions(
    i_cluster_name text)
RETURNS SETOF text
LANGUAGE plpgsql
AS $$
DECLARE
    v_min_priority integer;
    v_cluster_name text;
    v_cluster_id integer;
    v_dc text;
BEGIN
    IF i_cluster_name = 'meta_rw' THEN
        v_cluster_name := 'meta';
        v_min_priority := 0;
    ELSIF i_cluster_name = 'meta_ro' THEN
        v_cluster_name := 'meta';
        v_min_priority := 1;
    ELSIF i_cluster_name = 'db_rw' THEN
        v_cluster_name := 'db';
        v_min_priority := 0;
    ELSIF i_cluster_name = 'db_ro' THEN
        v_cluster_name := 'db';
        v_min_priority := 1;
    ELSE
        RAISE EXCEPTION 'Unknown cluster';
    END IF;

    SELECT dc INTO v_dc FROM plproxy.my_dc;
    SELECT cluster_id
        INTO v_cluster_id
        FROM plproxy.clusters
      WHERE name = v_cluster_name;

    RETURN QUERY
    WITH replicas_minimum AS (
        SELECT p.part_id,
               min(pr.priority) AS cur_min_prio
            FROM plproxy.parts p
            JOIN plproxy.clusters c
                ON (p.cluster_id = c.cluster_id)
            JOIN plproxy.priorities pr
                ON (p.part_id = pr.part_id)
          WHERE p.cluster_id = v_cluster_id
            AND pr.priority != 0
          GROUP BY p.part_id
          ORDER BY p.part_id
        ),
    masters_minimum AS (
        SELECT p.part_id,
               min(pr.priority) AS cur_min_prio
            FROM plproxy.parts p
            JOIN plproxy.clusters c
                ON (p.cluster_id = c.cluster_id)
            JOIN plproxy.priorities pr
                ON (p.part_id = pr.part_id)
          WHERE p.cluster_id = v_cluster_id
            AND (pr.priority = 0 OR pr.priority >= 100)
          GROUP BY p.part_id
          ORDER BY p.part_id
        ),
    local_masters AS (
        SELECT pr.part_id, pr.conn_id
            FROM plproxy.hosts h JOIN plproxy.priorities pr
                ON (pr.host_id = h.host_id)
            JOIN plproxy.parts p ON (pr.part_id = p.part_id)
          WHERE h.dc = v_dc
            AND pr.priority = 0
            AND p.cluster_id = v_cluster_id
    ),
    all_conn_ids AS (
        SELECT p.part_id, p.conn_id, p.priority
            FROM plproxy.priorities p
            JOIN replicas_minimum rm ON (p.part_id = rm.part_id)
            JOIN masters_minimum mm ON (p.part_id = mm.part_id)
          WHERE p.priority >= CASE WHEN rm.cur_min_prio >= 100 AND v_min_priority = 1 THEN 0
                                WHEN mm.cur_min_prio >= 1 AND v_min_priority = 0 THEN 100
                                ELSE v_min_priority
                              END
          ORDER BY p.part_id, p.priority
    ),
    conn_ids AS (
        SELECT DISTINCT part_id, first_value(conn_id) OVER w AS conn_id
            FROM all_conn_ids
          WINDOW w AS (PARTITION BY part_id ORDER BY priority)
        ),
    unsorted AS (
        SELECT ci.part_id, c.conn_string
            FROM plproxy.connections c
            JOIN conn_ids ci ON (ci.conn_id = c.conn_id)
          WHERE CASE WHEN v_min_priority > 0
                    THEN ci.part_id NOT IN (
                        SELECT part_id FROM local_masters
                    )
                    ELSE true
                END
        UNION ALL
        SELECT l.part_id, c.conn_string
            FROM plproxy.connections c
            JOIN local_masters l ON (c.conn_id = l.conn_id)
          WHERE CASE WHEN v_min_priority = 0
                    THEN l.part_id NOT IN (
                        SELECT part_id FROM conn_ids
                    )
                    ELSE true
                END
        )
    SELECT conn_string::text
        FROM unsorted
      ORDER BY part_id;

    RETURN;
END;
$$;

CREATE OR REPLACE FUNCTION plproxy.get_cluster_version (
    i_cluster_name text )
RETURNS integer
LANGUAGE plpgsql
AS $$
DECLARE
    ver integer;
BEGIN
    IF i_cluster_name IN ('meta_rw', 'meta_ro', 'db_rw', 'db_ro') THEN
        SELECT version INTO ver FROM plproxy.versions;
        RETURN ver;
    END IF;
    RAISE exception 'Unknown cluster';
END;
$$;

CREATE OR REPLACE FUNCTION plproxy.get_cluster_config(
    i_cluster_name text,
    out o_key text,
    out o_val text)
RETURNS SETOF record
LANGUAGE plpgsql
AS $$
BEGIN
    IF i_cluster_name NOT IN ('meta_rw', 'meta_ro', 'db_rw', 'db_ro') THEN
        RAISE EXCEPTION 'Unknown cluster';
    END IF;

    RETURN QUERY
    SELECT key::text AS o_key,
           value::text AS o_val
        FROM plproxy.config;
    RETURN;
END;
$$;

DROP FUNCTION IF EXISTS plproxy.select_part(i_key bigint);

CREATE OR REPLACE FUNCTION plproxy.get_connstring(
    i_db_type text,
    i_shard_id int DEFAULT NULL
)
RETURNS SETOF text
LANGUAGE sql STABLE AS $function$
    WITH full_conns AS (
        SELECT conn,
            row_number() OVER () AS pos
        FROM plproxy.get_cluster_partitions(i_db_type) f(conn)
    ),
    conns AS (
        SELECT conn,
            substring(conn from 'dbname=[^\ ]+') AS dbname,
            min(pos) AS pos
        FROM full_conns
        GROUP BY conn
        ORDER BY pos
    )
    SELECT CASE WHEN dbname ~ 'dbname=[^_]+__[0-9]+__[^_]+$'
        THEN (
            SELECT format('host=%s port=%s dbname=%s', db[1], db[2], db[3])
            FROM regexp_split_to_array(replace(dbname, 'dbname=', ''), '__') db
        )
        ELSE conn END
    FROM conns
    ORDER BY pos ASC
    LIMIT CASE WHEN i_shard_id IS NULL THEN NULL ELSE 1 END
    OFFSET CASE WHEN i_shard_id IS NULL THEN 0 ELSE i_shard_id END
$function$;

GRANT USAGE ON SCHEMA plproxy TO s3api;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL tables IN SCHEMA plproxy TO s3api;
GRANT EXECUTE ON ALL functions IN SCHEMA plproxy TO s3api;
GRANT ALL ON ALL sequences IN SCHEMA plproxy TO s3api;

GRANT USAGE ON SCHEMA plproxy TO s3api_ro;
GRANT SELECT ON ALL tables IN SCHEMA plproxy TO s3api_ro;
GRANT EXECUTE ON ALL functions IN SCHEMA plproxy TO s3api_ro;
GRANT SELECT ON ALL sequences IN SCHEMA plproxy TO s3api_ro;

GRANT USAGE ON SCHEMA plproxy TO s3api_list;
GRANT SELECT ON ALL tables IN SCHEMA plproxy TO s3api_list;
GRANT EXECUTE ON ALL functions IN SCHEMA plproxy TO s3api_list;
GRANT SELECT ON ALL sequences IN SCHEMA plproxy TO s3api_list;

GRANT USAGE ON SCHEMA plproxy TO s3cleanup;
GRANT SELECT ON ALL tables IN SCHEMA plproxy TO s3cleanup;
GRANT EXECUTE ON FUNCTION plproxy.get_connstring(text, int) TO s3cleanup;
