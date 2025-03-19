SELECT
    folder_id,
    folder_ext_id,
    cloud_id
FROM
    dbaas.folders
WHERE
    (%(folder_id)s IS NULL OR folder_id = %(folder_id)s)
    AND (%(folder_ext_id)s IS NULL OR folder_ext_id = %(folder_ext_id)s)
