CREATE OR REPLACE FUNCTION code.get_version_by_host(
    i_fqdn      text
) RETURNS SETOF code.version AS $$
SELECT component, major_version, minor_version, package_version, edition
  FROM (
    SELECT v.*, MAX(priority) OVER (PARTITION BY component) AS max_priority
      FROM dbaas.hosts
      JOIN dbaas.subclusters USING (subcid)
      JOIN dbaas.clusters USING (cid),
   LATERAL (
        SELECT component, major_version, minor_version, package_version, 'cid'::code.version_priority AS priority, edition
          FROM dbaas.versions
         WHERE cid = subclusters.cid

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'subcid'::code.version_priority AS priority, edition
          FROM dbaas.versions
         WHERE subcid = hosts.subcid

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'shard_id'::code.version_priority AS priority, edition
          FROM dbaas.versions
         WHERE shard_id = hosts.shard_id) v
     WHERE fqdn = i_fqdn
       AND code.visible(clusters)
  ) vp
WHERE priority = max_priority
$$ LANGUAGE SQL STABLE;
