CREATE OR REPLACE FUNCTION code.get_host(
    i_fqdn text
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.subclusters s
  JOIN dbaas.hosts h
 USING (subcid)
  JOIN dbaas.clusters c
 USING (cid)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id),
       code.format_host(h, c, s, g, d, f) fmt
 WHERE h.fqdn = i_fqdn;
$$ LANGUAGE SQL STABLE;
