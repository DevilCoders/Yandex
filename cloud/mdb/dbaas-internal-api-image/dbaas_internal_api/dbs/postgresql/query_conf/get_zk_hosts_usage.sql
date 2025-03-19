SELECT value->'data'->'pgsync'->'zk_hosts' as hosts, count(*) as c
  FROM dbaas.pillar p JOIN dbaas.clusters cl ON (p.cid = cl.cid)
  WHERE value->'data'->'pgsync' IS NOT NULL
    AND p.cid IS NOT NULL
    AND cl.type = 'postgresql_cluster'
    AND code.visible(cl)
  GROUP BY 1
