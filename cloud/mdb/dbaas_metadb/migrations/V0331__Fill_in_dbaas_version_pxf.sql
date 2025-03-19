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
    SELECT c.cid,
           NULL AS subcid,
           NULL AS shard_id,
           'pxf' AS component,
           v.major_version,
           '5.16.2-20',
           '5.16.2-20-yandex.1063.b7e35ae4' AS package_version,
           c.actual_rev,
           v.edition
    FROM dbaas.clusters c
      JOIN dbaas.versions v ON (c.cid = v.cid AND v.component='greenplum')
     WHERE dbaas.visible_cluster_status(status)
       AND c.status not in ('MODIFYING', 'MODIFY-ERROR') AND c.type='greenplum_cluster'

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
