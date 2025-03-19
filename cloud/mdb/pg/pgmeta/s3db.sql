CREATE TABLE clusters
(
    cluster_id  integer NOT NULL,
    name        text    NOT NULL,
    CONSTRAINT pk_clusters PRIMARY KEY (cluster_id)
);

CREATE TABLE parts
(
    part_id     integer NOT NULL,
    cluster_id  integer NOT NULL,
    CONSTRAINT fk_parts_cluster_id_clusters FOREIGN KEY (cluster_id)
            REFERENCES clusters (cluster_id) ON DELETE CASCADE,
    CONSTRAINT pk_parts PRIMARY KEY (part_id)
);

CREATE TABLE hosts
(
    host_id     integer NOT NULL,
    host_name   text    NOT NULL,
    dc          text,
    base_prio   smallint,
    prio_diff   smallint,
    CONSTRAINT pk_hosts PRIMARY KEY (host_id)
);

CREATE TABLE connections
(
    conn_id     integer NOT NULL,
    conn_string text    NOT NULL,
    CONSTRAINT pk_connections PRIMARY KEY (conn_id)
);

CREATE TABLE config
(
    key     text    NOT NULL,
    value   text,
    CONSTRAINT uk_config_key UNIQUE (key)
);

CREATE TABLE priorities
(
    part_id     integer     NOT NULL,
    host_id     integer     NOT NULL,
    conn_id     integer     NOT NULL,
    priority    smallint    DEFAULT 100 NOT NULL,
    CONSTRAINT pk_priorities PRIMARY KEY (part_id, host_id, conn_id),
    CONSTRAINT fk_priorities_part_id_parts FOREIGN KEY (part_id)
        REFERENCES parts (part_id) ON DELETE CASCADE,
    CONSTRAINT fk_priorities_host_id_hosts FOREIGN KEY (host_id)
        REFERENCES hosts (host_id) ON DELETE CASCADE,
    CONSTRAINT fk_priorities_conn_id_connections FOREIGN KEY (conn_id)
        REFERENCES connections (conn_id) ON DELETE CASCADE
);

GRANT USAGE ON SCHEMA public TO monitor;
GRANT INSERT, UPDATE, SELECT ON ALL TABLES IN SCHEMA public TO pgproxy;

CREATE OR REPLACE FUNCTION get_connections()
RETURNS TABLE (name text, shard_id int, dc text, conn_string text)
LANGUAGE SQL STABLE
AS $$
    WITH orig AS (
        SELECT name, shard_id, dc, conn_string, dbname
          FROM (
            SELECT name, part_id,
                row_number() OVER (PARTITION BY cluster_id ORDER BY part_id) - 1 AS shard_id
              FROM clusters JOIN parts USING (cluster_id)
          ) shards JOIN (
            SELECT conn_id, part_id, dc, conn_string,
                substring(conn_string FROM 'dbname=[^\ ]+') AS dbname
              FROM connections
                JOIN priorities USING (conn_id)
                JOIN hosts USING (host_id)
          ) conns USING (part_id)
    )
    SELECT name, shard_id::int, dc,
        CASE WHEN dbname ~ 'dbname=[^_]+__[0-9]+__[a-zA-Z0-9_-]+$' THEN (
            SELECT format('host=%s port=%s dbname=%s', db[1], db[2], db[3])
              FROM regexp_split_to_array(replace(dbname, 'dbname=', ''), '__') db
        )
        ELSE conn_string END
    FROM orig;
$$;


GRANT SELECT ON ALL TABLES IN SCHEMA public TO s3ro;
GRANT USAGE ON SCHEMA public TO s3ro;
