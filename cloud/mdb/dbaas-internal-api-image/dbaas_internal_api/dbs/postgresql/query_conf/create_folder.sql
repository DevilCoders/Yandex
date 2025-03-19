INSERT INTO dbaas.folders (
    folder_ext_id,
    cloud_id
) VALUES (
    %(folder_ext_id)s,
    %(cloud_id)s
) RETURNING
    folder_id,
    folder_ext_id,
    cloud_id
