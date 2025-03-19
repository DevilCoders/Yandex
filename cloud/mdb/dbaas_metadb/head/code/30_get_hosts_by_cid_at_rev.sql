CREATE OR REPLACE FUNCTION code.get_hosts_by_cid_at_rev(
    i_cid text,
    i_rev bigint
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.clusters c
  JOIN dbaas.clusters_revs cr
 USING (cid)
  JOIN dbaas.subclusters_revs s
 USING (cid, rev)
  JOIN dbaas.hosts_revs h
 USING (subcid, rev)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id),
       code.format_host(h, c, cr, s, g, d, f) fmt
 WHERE c.cid = i_cid
   AND cr.rev = i_rev;
$$ LANGUAGE SQL STABLE;
