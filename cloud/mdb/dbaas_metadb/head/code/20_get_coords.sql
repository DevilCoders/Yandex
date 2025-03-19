CREATE OR REPLACE FUNCTION code.get_coords(
    i_cid text
) RETURNS code.cluster_coords AS $$
SELECT (i_cid, subcids, shard_ids, fqdns)::code.cluster_coords
  FROM (
      SELECT array_agg(subcid) AS subcids
        FROM dbaas.subclusters
       WHERE subclusters.cid = i_cid) sc,
  LATERAL (
      SELECT array_agg(shard_id) AS shard_ids
        FROM dbaas.shards
       WHERE shards.subcid = ANY(subcids)) sha,
  LATERAL (
      SELECT array_agg(fqdn) AS fqdns
        FROM dbaas.hosts
       WHERE hosts.subcid = ANY(subcids)) h;
$$ LANGUAGE SQL STABLE;
