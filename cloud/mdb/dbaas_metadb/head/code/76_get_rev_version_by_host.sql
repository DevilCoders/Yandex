CREATE OR REPLACE FUNCTION code.get_rev_version_by_host(
    i_fqdn      text,
    i_rev       bigint
) RETURNS SETOF code.version AS $$
SELECT component, major_version, minor_version, package_version, edition
  FROM (
    SELECT v.*, MAX(priority) OVER (PARTITION BY component) AS max_priority
      FROM dbaas.hosts_revs
      JOIN dbaas.subclusters_revs USING (subcid)
      JOIN dbaas.clusters USING (cid),
   LATERAL (
        SELECT component, major_version, minor_version, package_version, 'cid'::code.version_priority AS priority, edition
          FROM dbaas.versions_revs
         WHERE cid = subclusters_revs.cid
           AND rev = i_rev

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'subcid'::code.version_priority AS priority, edition
          FROM dbaas.versions_revs
         WHERE subcid = hosts_revs.subcid
           AND rev = i_rev

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'shard_id'::code.version_priority AS priority, edition
          FROM dbaas.versions_revs
         WHERE shard_id = hosts_revs.shard_id
           AND rev = i_rev) v
    WHERE fqdn = i_fqdn
      AND code.visible(clusters)
      AND hosts_revs.rev = i_rev
      AND subclusters_revs.rev = i_rev
  ) vp
WHERE priority = max_priority
$$ LANGUAGE SQL STABLE;
