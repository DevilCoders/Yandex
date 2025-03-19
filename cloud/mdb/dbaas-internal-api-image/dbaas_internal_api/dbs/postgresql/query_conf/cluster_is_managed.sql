SELECT
    code.managed(c) AS managed
FROM
    dbaas.clusters c
WHERE
    c.cid = %(cid)s
