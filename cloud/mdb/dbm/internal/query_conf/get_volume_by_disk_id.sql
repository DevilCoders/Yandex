SELECT
    container,
    path
FROM
    mdb.volumes
WHERE
    disk_id = %(disk_id)s
