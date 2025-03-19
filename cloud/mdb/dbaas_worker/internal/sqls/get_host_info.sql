SELECT c.cid,
       c.type,
       h.vtype_id,
       pg.pg_id,
       pg.local_id,
       pg.placement_group_id
FROM dbaas.hosts h
JOIN dbaas.subclusters sc
  ON h.subcid = sc.subcid
JOIN dbaas.clusters c
  ON sc.cid = c.cid
LEFT JOIN dbaas.placement_groups pg
  ON (c.cid = pg.cid AND h.fqdn = pg.fqdn)
WHERE h.fqdn = %(fqdn)s
