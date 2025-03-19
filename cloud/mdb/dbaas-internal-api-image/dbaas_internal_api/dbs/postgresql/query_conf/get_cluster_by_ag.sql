SELECT
    c.cid,
    c.folder_id,
    c.name,
    c.type,
    c.env
FROM
    dbaas.clusters c
    JOIN dbaas.alert_group a USING (cid)
WHERE
    a.alert_group = %(ag_id)s
    AND code.visible(c)
    AND code.managed(c)
