CREATE OR REPLACE FUNCTION code.get_hosts_by_cid(
    i_cid        text,
    i_visibility code.visibility DEFAULT 'visible'
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.clusters c
  JOIN dbaas.subclusters s
 USING (cid)
  JOIN dbaas.hosts h
 USING (subcid)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id),
       code.format_host(h, c, s, g, d, f) fmt
 WHERE c.cid = i_cid
   AND code.match_visibility(c, i_visibility);
$$ LANGUAGE SQL STABLE;
