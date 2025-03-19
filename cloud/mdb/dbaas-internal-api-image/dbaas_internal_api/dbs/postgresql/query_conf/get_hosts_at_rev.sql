SELECT
    h.subcid,
    h.shard_id,
    sh.name AS shard_name,
    space_limit,
    h.flavor AS flavor,
    g.name,
    fqdn,
    assign_public_ip,
    f.name AS flavor_name,
    f.vtype,
    vtype_id,
    disk_type_id,
    subnet_id,
    cast(roles AS text[]) AS roles
  FROM dbaas.subclusters_revs s
  JOIN dbaas.hosts_revs h
 USING (subcid, rev)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id)
  LEFT JOIN dbaas.shards_revs sh
    ON (sh.subcid = s.subcid AND sh.rev = s.rev)
 WHERE s.cid = %(cid)s
   AND s.rev = %(rev)s
