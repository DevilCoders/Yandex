CREATE OR REPLACE FUNCTION code.get_cloud_feature_flags(
    i_cloud_id bigint
) RETURNS text[] AS $$
SELECT ARRAY(
    SELECT flag_name
        FROM dbaas.default_feature_flags
    UNION
    SELECT flag_name
        FROM dbaas.cloud_feature_flags
        WHERE cloud_id = i_cloud_id
    ORDER BY flag_name);
$$ LANGUAGE SQL STABLE;
