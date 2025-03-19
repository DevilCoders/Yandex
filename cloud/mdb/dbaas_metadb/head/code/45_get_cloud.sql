CREATE OR REPLACE FUNCTION code.get_cloud(
    i_cloud_id     bigint,
    i_cloud_ext_id text
) RETURNS SETOF code.cloud AS $$
SELECT fmt.*
  FROM dbaas.clouds c,
       code.format_cloud(c) fmt
 WHERE (i_cloud_id IS NULL OR c.cloud_id = i_cloud_id)
   AND (i_cloud_ext_id IS NULL OR c.cloud_ext_id = i_cloud_ext_id);
$$ LANGUAGE SQL STABLE;
