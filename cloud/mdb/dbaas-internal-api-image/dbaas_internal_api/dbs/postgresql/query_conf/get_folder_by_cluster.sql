SELECT
    f.folder_id,
    f.folder_ext_id,
    f.cloud_id
FROM
    dbaas.folders f,
    dbaas.clusters c
WHERE
    f.folder_id = c.folder_id
    AND (%(cid)s IS NULL OR c.cid = %(cid)s)
    AND (%(cluster_name)s IS NULL OR c.name = %(cluster_name)s)
    AND code.match_visibility(c, %(visibility)s)
