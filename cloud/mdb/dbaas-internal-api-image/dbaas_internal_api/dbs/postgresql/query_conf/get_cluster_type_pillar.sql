SELECT
    value
FROM
    dbaas.cluster_type_pillar
WHERE
    type = %(cluster_type)s
