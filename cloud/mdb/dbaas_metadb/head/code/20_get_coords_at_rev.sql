CREATE OR REPLACE FUNCTION code.get_coords_at_rev(
    i_cid text,
    i_rev bigint
) RETURNS code.cluster_coords AS $$
SELECT (i_cid, subcids, shard_ids, fqdns)::code.cluster_coords
  FROM (
      SELECT array_agg(subcid) AS subcids
        FROM dbaas.subclusters_revs
       WHERE subclusters_revs.cid = i_cid
         AND rev = i_rev) sc,
  LATERAL (
      SELECT array_agg(shard_id) AS shard_ids
        FROM dbaas.shards_revs
       WHERE shards_revs.subcid = ANY(subcids)
         AND rev = i_rev) sha,
  LATERAL (
      SELECT array_agg(fqdn) AS fqdns
        FROM dbaas.hosts_revs
       WHERE hosts_revs.subcid = ANY(subcids)
         AND rev = i_rev) h;
$$ LANGUAGE SQL STABLE;