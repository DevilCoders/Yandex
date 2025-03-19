CREATE OR REPLACE FUNCTION code.get_hosts_by_shard(
    i_shard_id text,
    i_visibility code.visibility DEFAULT 'visible'
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.shards s
  JOIN dbaas.subclusters sc USING (subcid)
  JOIN dbaas.clusters c USING (cid)
  JOIN dbaas.hosts h USING (shard_id)
  JOIN dbaas.geo g USING (geo_id)
  JOIN dbaas.disk_type d USING (disk_type_id)
  JOIN dbaas.flavors f ON (h.flavor = f.id),
       code.format_host(h, c, sc, g, d, f) fmt
 WHERE s.shard_id = i_shard_id
   AND code.match_visibility(c, i_visibility);
$$ LANGUAGE SQL STABLE;
