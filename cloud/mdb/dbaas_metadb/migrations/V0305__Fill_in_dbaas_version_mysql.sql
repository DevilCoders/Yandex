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
           value->'data'->'mysql'->'version'->>'major_human' AS name,
          'mysql' AS component
      FROM dbaas.clusters
      JOIN dbaas.pillar USING (cid)
     WHERE dbaas.visible_cluster_status(status)
       AND (status = 'RUNNING' OR status = 'STOPPED') AND type = 'mysql_cluster'
    ) c
    JOIN dbaas.default_versions USING (env, type, name, component)
      ON CONFLICT DO NOTHING
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
  FROM dfr_ins
    ON CONFLICT DO NOTHING;
