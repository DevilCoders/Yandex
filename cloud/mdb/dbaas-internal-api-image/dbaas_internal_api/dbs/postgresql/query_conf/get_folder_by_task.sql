SELECT
    f.folder_id,
    f.folder_ext_id,
    f.cloud_id
FROM
    dbaas.folders f
WHERE
    f.folder_id IN (
        SELECT folder_id
        FROM code.get_folder_id_by_operation(%(task_id)s) folder_id)
