CREATE OR REPLACE FUNCTION code.get_rev_pillar_by_host(
    i_fqdn      text,
    i_rev       bigint,
    i_target_id text DEFAULT NULL
) RETURNS SETOF code.pillar_with_priority AS $$
WITH host AS (
    SELECT cid,
           type,
           subcid,
           roles,
           shard_id
      FROM dbaas.hosts_revs h
      JOIN dbaas.subclusters_revs sc USING (subcid)
      JOIN dbaas.clusters USING (cid)
     WHERE fqdn = i_fqdn
       AND code.visible(clusters)
       AND h.rev = i_rev
       AND sc.rev = i_rev
)
SELECT value, 'default'::code.pillar_priority
  FROM dbaas.default_pillar
 WHERE id = 1

 UNION ALL

SELECT value, 'cluster_type'::code.pillar_priority
  FROM dbaas.cluster_type_pillar
 WHERE type = (SELECT type FROM host)

 UNION ALL

SELECT value, 'role'::code.pillar_priority
  FROM dbaas.role_pillar p
  JOIN host h ON (p.type = h.type AND p.role = ANY(h.roles))

 UNION ALL

SELECT value, 'cid'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE cid = (SELECT cid FROM host)
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_cid'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE cid = (SELECT cid FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'subcid'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE subcid = (SELECT subcid FROM host)
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_subcid'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE subcid = (SELECT subcid FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'shard_id'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE shard_id = (SELECT shard_id FROM host)
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_shard_id'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE shard_id = (SELECT shard_id FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'fqdn'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE fqdn = i_fqdn
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_fqdn'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE fqdn = i_fqdn
   AND target_id = i_target_id
$$ LANGUAGE SQL STABLE;
