PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$str = ($field) -> (CAST($field AS String));

$resource_memberships_hist_parsed = (
SELECT
    $str(resource_id)                                       AS iam_resource_id,
    $str(resource_type)                                     AS iam_resource_type,
    $str(subject_id)                                        AS iam_user_id,
    modified_at                                             AS modified_ts,
    $from_utc_ts_to_msk_dt(modified_at)                     AS modified_dttm_local,
    deleted_at                                              AS deleted_ts,
    $from_utc_ts_to_msk_dt(deleted_at)                      AS deleted_dttm_local,
    $str(JSON_VALUE(cast(metadata as JSON), "$.user_id"))   AS metadata_user_id,
FROM $select_transfer_manager_table($src_folder, $cluster) AS a
LEFT JOIN $dst_table as b
ON $str(a.subject_id)=b.iam_user_id AND a.modified_at=b.modified_ts AND $str(a.resource_id) = b.iam_resource_id
WHERE b.iam_user_id IS NULL
);

$result = (
SELECT
    iam_resource_id,
    iam_resource_type,
    iam_user_id,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    metadata_user_id
FROM $dst_table
UNION ALL
SELECT
    iam_resource_id,
    iam_resource_type,
    iam_user_id,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    metadata_user_id
FROM $resource_memberships_hist_parsed
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_resource_id,
    iam_resource_type,
    iam_user_id,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    metadata_user_id
FROM
    $result
ORDER BY
    iam_resource_id, iam_resource_type, iam_user_id, modified_ts;
