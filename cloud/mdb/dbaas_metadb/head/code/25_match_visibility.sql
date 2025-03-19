CREATE OR REPLACE FUNCTION code.match_visibility(
    c dbaas.clusters,
    v code.visibility
) RETURNS boolean AS $$
SELECT
    CASE v
        WHEN 'all' THEN true
        WHEN 'visible+deleted' THEN c.status NOT IN ('PURGING', 'PURGED', 'PURGE-ERROR')
        ELSE code.visible(c)
    END;
$$ LANGUAGE SQL IMMUTABLE;
