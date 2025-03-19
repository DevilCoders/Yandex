CREATE OR REPLACE FUNCTION code.lock_cloud(
    i_cloud_id bigint
) RETURNS SETOF code.cloud AS $$
SELECT fmt.*
  FROM dbaas.clouds c,
       code.format_cloud(c) fmt
 WHERE c.cloud_id = i_cloud_id
   FOR UPDATE;
$$ LANGUAGE SQL;
