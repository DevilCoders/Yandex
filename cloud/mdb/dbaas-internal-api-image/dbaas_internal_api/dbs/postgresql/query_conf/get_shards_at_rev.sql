SELECT sh.subcid,
       sh.shard_id,
       sh.name,
       coalesce(pl.value, '{}'::jsonb) "value"
  FROM dbaas.shards_revs sh
  JOIN dbaas.subclusters_revs sc
 USING (subcid, rev)
  LEFT JOIN dbaas.pillar_revs pl
 USING (shard_id, rev)
 WHERE sc.cid = %(cid)s
 AND rev = %(rev)s
 ORDER BY sh.name
