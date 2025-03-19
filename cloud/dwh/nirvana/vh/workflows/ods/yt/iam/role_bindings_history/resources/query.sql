PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt, $get_datetime_secs;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$str = ($field) -> (CAST($field AS String));
$uint64_secs_to_timestamp = ($field) -> ($get_datetime_secs(CAST($field AS uint32)));


$role_bindings_hist_primary_processed = (
SELECT
    $uint64_secs_to_timestamp(a.modified_at)                             AS modified_ts,
    $from_utc_ts_to_msk_dt($uint64_secs_to_timestamp(a.modified_at))     AS modified_dttm_local,
    $str(a.resource_id)                                                  AS iam_resource_id,
    $str(a.role_slug)                                                    AS role_slug,
    $str(a.subject_id)                                                   AS iam_subject_id,
    $uint64_secs_to_timestamp(a.deleted_at)                              AS deleted_ts,
    $from_utc_ts_to_msk_dt($uint64_secs_to_timestamp(a.deleted_at))      AS deleted_dttm_local,
    $str(a.id)                                                           AS iam_binding_id,
    a.is_system                                                          AS is_system,
    $str(a.resource_cloud_id)                                            AS iam_resource_cloud_id,
    $str(a.resource_type)                                                AS resource_type,
    $str(a.subject_type)                                                 AS subject_type,
    $str(a.managed_by)                                                   AS managed_by,
    $str(a.top_level_resource_id)                                        AS iam_top_level_resource_id,
    $str(a.top_level_resource_type)                                      AS top_level_resource_type,
    $str(JSON_VALUE(CAST(a.metadata as JSON), "$.user_id"))              AS metadata_user_id
FROM $select_transfer_manager_table($src_folder, $cluster) AS a
LEFT JOIN $dst_table as b
ON $uint64_secs_to_timestamp(a.modified_at) = b.modified_ts
AND $uint64_secs_to_timestamp(a.deleted_at) = b.deleted_ts
AND $str(a.id) = b.iam_binding_id
WHERE b.iam_binding_id IS NULL
);

$result = (
SELECT
    modified_ts,
    modified_dttm_local,
    iam_resource_id,
    role_slug,
    iam_subject_id,
    deleted_ts,
    deleted_dttm_local,
    iam_binding_id,
    is_system,
    iam_resource_cloud_id,
    resource_type,
    subject_type,
    managed_by,
    iam_top_level_resource_id,
    top_level_resource_type,
    metadata_user_id
FROM $dst_table
UNION ALL
SELECT
    modified_ts,
    modified_dttm_local,
    iam_resource_id,
    role_slug,
    iam_subject_id,
    deleted_ts,
    deleted_dttm_local,
    iam_binding_id,
    is_system,
    iam_resource_cloud_id,
    resource_type,
    subject_type,
    managed_by,
    iam_top_level_resource_id,
    top_level_resource_type,
    metadata_user_id
FROM $role_bindings_hist_primary_processed
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    modified_ts,
    modified_dttm_local,
    iam_resource_id,
    role_slug,
    iam_subject_id,
    deleted_ts,
    deleted_dttm_local,
    iam_binding_id,
    is_system,
    iam_resource_cloud_id,
    resource_type,
    subject_type,
    managed_by,
    iam_top_level_resource_id,
    top_level_resource_type,
    metadata_user_id
FROM
    $result
ORDER BY
    modified_ts, iam_resource_id, role_slug, iam_subject_id;
