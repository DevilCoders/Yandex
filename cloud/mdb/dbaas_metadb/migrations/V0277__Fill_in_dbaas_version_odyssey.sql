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
           'odyssey' AS component,
           major_version,
           'latest',
           '1886-8318dec-yandex200' AS package_version,
           actual_rev,
           edition
    FROM dbaas.clusters
      JOIN dbaas.versions USING (cid)
     WHERE dbaas.visible_cluster_status(status)
       AND status not in ('MODIFYING', 'MODIFY-ERROR') AND type='postgresql_cluster'

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
