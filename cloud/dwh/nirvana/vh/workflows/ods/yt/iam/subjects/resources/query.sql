PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

PRAGMA DqEngine="disable";

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt, $get_datetime_secs;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$src_history_folder = {{ param["source_history_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$pii_dst_table = {{ param["PII_destination_path"] -> quote() }};

$str = ($field) -> (CAST($field AS String));
$snapshot_history = SELECT * FROM $select_transfer_manager_table($src_history_folder, $cluster);
$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);


-- Restore deleted subjects from history table
$subjects_history = (
SELECT
    id                                                  AS id,
    subject_type                                        AS subject_type,
    created_at                                          AS created_at,
    modified_at                                         AS modified_at,
    deleted_at                                          AS deleted_at,
    cloud_id                                            AS cloud_id,
    reference                                           AS reference,
    eula                                                AS eula,
    privacy_policy                                      AS privacy_policy,
    email                                               AS email,
    login                                               AS login,
    federation_id                                       AS federation_id,
    external_id                                         AS external_id,
    settings                                            AS settings
FROM (
    SELECT
        id                                                  AS id,
        subject_type                                        AS subject_type,
        created_at                                          AS created_at,
        modified_at                                         AS modified_at,
        deleted_at                                          AS deleted_at,
        cloud_id                                            AS cloud_id,
        reference                                           AS reference,
        eula                                                AS eula,
        privacy_policy                                      AS privacy_policy,
        email                                               AS email,
        login                                               AS login,
        federation_id                                       AS federation_id,
        external_id                                         AS external_id,
        settings                                            AS settings,
        ROW_NUMBER() OVER (partition by id order by modified_at desc) as rn
    FROM $snapshot_history)
WHERE rn = 1
);

$subjects = (
SELECT
    id                                                  AS id,
    subject_type                                        AS subject_type,
    created_at                                          AS created_at,
    modified_at                                         AS modified_at,
    cloud_id                                            AS cloud_id,
    reference                                           AS reference,
    eula                                                AS eula,
    privacy_policy                                      AS privacy_policy,
    email                                               AS email,
    login                                               AS login,
    federation_id                                       AS federation_id,
    external_id                                         AS external_id,
    settings                                            AS settings,
FROM $snapshot
WHERE id NOT IN (SELECT id FROM $subjects_history)
);

$subjects_restored = (
SELECT
    id                                                  AS id,
    subject_type                                        AS subject_type,
    created_at                                          AS created_at,
    modified_at                                         AS modified_at,
    cloud_id                                            AS cloud_id,
    reference                                           AS reference,
    eula                                                AS eula,
    privacy_policy                                      AS privacy_policy,
    email                                               AS email,
    login                                               AS login,
    federation_id                                       AS federation_id,
    external_id                                         AS external_id,
    settings                                            AS settings,
FROM $subjects
UNION ALL
SELECT
    id                                                  AS id,
    subject_type                                        AS subject_type,
    created_at                                          AS created_at,
    modified_at                                         AS modified_at,
    deleted_at                                          AS deleted_at,
    cloud_id                                            AS cloud_id,
    reference                                           AS reference,
    eula                                                AS eula,
    privacy_policy                                      AS privacy_policy,
    email                                               AS email,
    login                                               AS login,
    federation_id                                       AS federation_id,
    external_id                                         AS external_id,
    settings                                            AS settings,
FROM $subjects_history
);

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
    FROM $subjects_restored
);

$result = (
SELECT
    iam_subject_id                                             AS iam_subject_id,
    subject_type                                               AS subject_type,
    created_ts                                                 AS created_ts,
    $from_utc_ts_to_msk_dt(created_ts)                         AS created_dttm_local,
    modified_ts                                                AS modified_ts,
    $from_utc_ts_to_msk_dt(modified_ts)                        AS modified_dttm_local,
    deleted_ts                                                 AS deleted_ts,
    $from_utc_ts_to_msk_dt(deleted_ts)                         AS deleted_dttm_local,
    eula                                                       AS eula,
    privacy_policy                                             AS privacy_policy,
    external_id                                                AS external_id,
    iam_cloud_id                                               AS iam_cloud_id,
    JSON_VALUE(settings_json, "$.orgId")                       AS iam_organization_id,
    JSON_VALUE(settings_json, "$.folderId")                    AS iam_folder_id,
    JSON_VALUE(settings_json, "$.experiments.asideNavigation") AS iam_aside_navigation_flag
FROM
    $subjects_prepared
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
    iam_folder_id,
    iam_aside_navigation_flag
FROM $dst_table
WHERE iam_subject_id NOT IN (SELECT iam_subject_id FROM $subjects_prepared)
);

$pii_result = (
SELECT
    iam_subject_id                                                  AS iam_subject_id,
    iam_federation_id                                               AS iam_federation_id,
    NVL(email, JSON_VALUE(settings_json, "$.email"))                AS email,
    NVL(login, JSON_VALUE(settings_json, "$.login"))                AS login,
    JSON_VALUE(settings_json, "$.phone")                            AS phone,
FROM
    $subjects_prepared
UNION ALL
SELECT
    iam_subject_id,
    iam_federation_id,
    email,
    login,
    phone
FROM $pii_dst_table
WHERE iam_subject_id NOT IN (SELECT iam_subject_id FROM $subjects_prepared)
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
    iam_folder_id,
    iam_aside_navigation_flag
FROM
    $result
ORDER BY
    iam_subject_id;


INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT
    iam_subject_id,
    iam_federation_id,
    email,
    login,
    phone
FROM
    $pii_result
ORDER BY
    iam_subject_id;
