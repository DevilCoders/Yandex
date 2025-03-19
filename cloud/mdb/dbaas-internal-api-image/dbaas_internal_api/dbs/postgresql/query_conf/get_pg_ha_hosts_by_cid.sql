SELECT 
    h.fqdn
FROM 
    dbaas.hosts h
    JOIN dbaas.subclusters 
    USING (subcid) 
    JOIN dbaas.clusters c
    USING (cid)
    JOIN dbaas.pillar p
    USING (fqdn)
WHERE 
    c.cid = %(cid)s
AND 
    p.value->'data'->'pgsync'->>'replication_source' IS NULL