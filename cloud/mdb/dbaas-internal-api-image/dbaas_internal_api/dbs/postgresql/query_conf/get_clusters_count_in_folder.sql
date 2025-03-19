SELECT
    count(*)
FROM
    dbaas.clusters
WHERE
    folder_id = %(folder_id)s
    AND code.visible(clusters)
    AND type = %(cluster_type)s
