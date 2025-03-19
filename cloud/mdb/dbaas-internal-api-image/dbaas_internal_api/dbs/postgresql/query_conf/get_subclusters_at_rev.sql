SELECT sc.subcid,
       sc.cid,
       sc.name,
       cast(sc.roles AS text[]) AS roles,
       coalesce(pl.value, '{}'::jsonb) "value"
  FROM dbaas.subclusters_revs sc
  LEFT JOIN dbaas.pillar_revs pl
 USING (subcid, rev)
 WHERE sc.cid = %(cid)s
   AND rev = %(rev)s
