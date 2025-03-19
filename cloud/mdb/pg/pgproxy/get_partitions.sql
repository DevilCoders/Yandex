CREATE OR REPLACE FUNCTION plproxy.get_partitions()
RETURNS TABLE (shard_id int, role text, conn_string text)
LANGUAGE SQL STABLE
SECURITY DEFINER
AS $$
WITH orig AS (
    SELECT
        ((row_number() OVER ()) - 1)::int AS shard_id,
        'master'::text AS role,
        conn_string,
        substring(conn_string from 'dbname=[^\ ]+') AS dbname
      FROM plproxy.get_cluster_partitions('rw') conn_string
    UNION ALL
    SELECT
        ((row_number() OVER ()) - 1)::int AS shard_id,
        'replica'::text AS role,
        conn_string,
        substring(conn_string from 'dbname=[^\ ]+') AS dbname
      FROM plproxy.get_cluster_partitions('ro') conn_string
)
SELECT shard_id, role,
    CASE WHEN dbname ~ 'dbname=[^_]+__[0-9]+__[a-zA-Z0-9_-]+$' THEN (
        SELECT format('host=%s port=%s dbname=%s', db[1], db[2], db[3])
          FROM regexp_split_to_array(replace(dbname, 'dbname=', ''), '__') db
    )
    ELSE conn_string END
  FROM orig;
$$
