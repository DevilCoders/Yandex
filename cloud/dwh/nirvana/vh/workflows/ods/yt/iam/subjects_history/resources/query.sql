PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

PRAGMA DqEngine="disable";

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt, $get_datetime_secs;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$pii_dst_table = {{ param["PII_destination_path"] -> quote() }};

$str = ($field) -> (CAST($field AS String));
$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

$subjects_prepared = (
    SELECT
        $str(id)                                                    AS iam_subject_id,
        $str(subject_type)                                          AS subject_type,
        $get_datetime_secs(CAST(created_at AS uint32))              AS created_ts,
        $get_datetime_secs(CAST(modified_at AS uint32))             AS modified_ts,
        $get_datetime_secs(CAST(deleted_at AS uint32))              AS deleted_ts,
        $str(cloud_id)                                              AS iam_cloud_id,
        $str(reference)                                             AS reference,
        $str(eula)                                                  AS eula,
        $str(privacy_policy)                                        AS privacy_policy,
        $str(email)                                                 AS email,
        $str(login)                                                 AS login,
        $str(federation_id)                                         AS iam_federation_id,
        $str(external_id)                                           AS external_id,
        CAST(settings AS JSON)                                      AS settings_json,
    FROM $snapshot
);

$new_subjects_prepared = (
SELECT
    a.iam_subject_id                                  AS iam_subject_id,
    a.subject_type                                    AS subject_type,
    a.created_ts                                      AS created_ts,
    $from_utc_ts_to_msk_dt(a.created_ts)              AS created_dttm_local,
    a.modified_ts                                     AS modified_ts,
    $from_utc_ts_to_msk_dt(a.modified_ts)             AS modified_dttm_local,
    a.deleted_ts                                      AS deleted_ts,
    $from_utc_ts_to_msk_dt(a.deleted_ts)              AS deleted_dttm_local,
    a.eula                                            AS eula,
    a.privacy_policy                                  AS privacy_policy,
    a.external_id                                     AS external_id,
    a.iam_cloud_id                                    AS iam_cloud_id,
    JSON_VALUE(a.settings_json, "$.orgId")            AS iam_organization_id,
    JSON_VALUE(a.settings_json, "$.folderId")         AS iam_folder_id
FROM
    $subjects_prepared as a
LEFT JOIN $dst_table as b
ON a.iam_subject_id = b.iam_subject_id
AND a.modified_ts = b.modified_ts
WHERE b.iam_subject_id IS NULL
);

$result = (
SELECT
    iam_subject_id,
    subject_type,
    created_ts,
    created_dttm_local,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    eula,
    privacy_policy,
    external_id,
    iam_cloud_id,
    iam_organization_id,
    iam_folder_id
FROM $dst_table
UNION ALL
SELECT
    iam_subject_id,
    subject_type,
    created_ts,
    created_dttm_local,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    eula,
    privacy_policy,
    external_id,
    iam_cloud_id,
    iam_organization_id,
    iam_folder_id
FROM $new_subjects_prepared
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_subject_id,
    subject_type,
    created_ts,
    created_dttm_local,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    eula,
    privacy_policy,
    external_id,
    iam_cloud_id,
    iam_organization_id,
    iam_folder_id
FROM $result
ORDER BY
    iam_subject_id;


INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT
    iam_subject_id                                                  AS iam_subject_id,
    iam_federation_id                                               AS iam_federation_id,
    deleted_ts,
    modified_ts,
    NVL(email, JSON_VALUE(settings_json, "$.email"))                AS email,
    NVL(login, JSON_VALUE(settings_json, "$.login"))                AS login,
    JSON_VALUE(settings_json, "$.phone")                            AS phone,
FROM
    $subjects_prepared
ORDER BY
    iam_subject_id;
