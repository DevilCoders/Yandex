DO
$do$
    DECLARE
        c dbaas.clusters;
    BEGIN
        FOR c IN (
            SELECT *
            FROM dbaas.clusters
            WHERE type = 'mongodb_cluster'
              AND status IN ('RUNNING', 'STOPPED')
              AND NOT EXISTS (
                SELECT 1
                  FROM dbaas.versions
                 WHERE versions.cid = clusters.cid)
            )
        LOOP
            WITH dfr_ins AS (
                INSERT INTO dbaas.versions_revs (
                       cid,
                       subcid,
                       shard_id,
                       component,
                       major_version,
                       minor_version,
                       package_version,
                       rev,
                       edition
                )
                SELECT cid,
                       NULL AS subcid,
                       NULL AS shard_id,
                       component,
                       major_version,
                       minor_version,
                       package_version,
                       actual_rev,
                       edition
                FROM (
                SELECT cid,
                       env,
                       type,
                       actual_rev,
                       value->'data'->'mongodb'->'version'->>'major_human' AS name,
                      'mongodb' AS component
                  FROM dbaas.clusters
                  JOIN dbaas.pillar USING (cid)
                 WHERE clusters.cid = (c).cid
                ) pv
                JOIN dbaas.default_versions USING (env, type, name, component)
                RETURNING *
            )
            INSERT INTO dbaas.versions (
                   cid,
                   subcid,
                   shard_id,
                   component,
                   major_version,
                   minor_version,
                   package_version,
                   edition
            )
            SELECT cid,
                   subcid,
                   shard_id,
                   component,
                   major_version,
                   minor_version,
                   package_version,
                   edition
              FROM dfr_ins;
 
            PERFORM code.add_finished_operation_for_current_rev(
                i_operation_id => gen_random_uuid()::text,
                i_cid => c.cid,
                i_folder_id => c.folder_id,
                i_operation_type => 'mongodb_cluster_modify',
                i_metadata => '{}'::jsonb,
                i_user_id => 'migration',
                i_version => 2,
                i_hidden => true,
                i_rev => c.actual_rev);
        END LOOP;
    END
$do$;
