SELECT
    c.cid,
    c.folder_id,
    c.name,
    c.type,
    c.env
FROM
    dbaas.clusters c
    JOIN dbaas.subclusters sc USING (cid)
    JOIN dbaas.hosts h USING (subcid)
WHERE
    h.fqdn = %(fqdn)s
    AND code.visible(c)
    AND NOT code.managed(c)
