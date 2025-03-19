PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

PRAGMA DqEngine="disable";

IMPORT `tables` SYMBOLS $select_datatransfer_snapshot_table;
IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt, $get_datetime_secs;
IMPORT `helpers` SYMBOLS $to_str, $get_md5;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$pii_dst_table = {{ param["PII_destination_path"] -> quote() }};

$offset = 1;

$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($src_folder, $cluster, $offset);

$prepared = (
    SELECT
        $to_str(a.id)                                                               AS iam_service_account_id,
        $to_str(a.status)                                                           AS iam_service_account_status,
        $to_str(a.description)                                                      AS iam_service_account_description,
        $get_datetime_secs(CAST(a.created_at AS uint32))                            AS created_ts,
        $from_utc_ts_to_msk_dt($get_datetime_secs(CAST(a.created_at AS uint32)))    AS created_dttm_local,
        $get_datetime_secs(CAST(a.modified_at AS uint32))                           AS modified_ts,
        $from_utc_ts_to_msk_dt($get_datetime_secs(CAST(a.modified_at AS uint32)))   AS modified_dttm_local,
        $get_datetime_secs(CAST(a.deleted_at AS uint32))                            AS deleted_ts,
        $from_utc_ts_to_msk_dt($get_datetime_secs(CAST(a.deleted_at AS uint32)))    AS deleted_dttm_local,
        $to_str(a.cloud_id)                                                         AS iam_cloud_id,
        $to_str(a.folder_id)                                                        AS iam_folder_id,
        CAST(a.labels AS JSON)                                                      AS iam_labels,
        $to_str(a.login)                                                            AS login
    FROM $snapshot AS a
);

$result = (
    SELECT
        iam_service_account_id,
        iam_service_account_status,
        iam_service_account_description_hash,
        created_ts,
        created_dttm_local,
        modified_ts,
        modified_dttm_local,
        deleted_ts,
        deleted_dttm_local,
        iam_cloud_id,
        iam_folder_id,
        iam_labels,
        login_hash
    FROM $dst_table
    UNION ALL
    SELECT
        a.iam_service_account_id                        AS iam_service_account_id,
        a.iam_service_account_status                    AS iam_service_account_status,
        $get_md5(a.iam_service_account_description)     AS iam_service_account_description_hash,
        a.created_ts                                    AS created_ts,
        a.created_dttm_local                            AS created_dttm_local,
        a.modified_ts                                   AS modified_ts,
        a.modified_dttm_local                           AS modified_dttm_local,
        a.deleted_ts                                    AS deleted_ts,
        a.deleted_dttm_local                            AS deleted_dttm_local,
        a.iam_cloud_id                                  AS iam_cloud_id,
        a.iam_folder_id                                 AS iam_folder_id,
        a.iam_labels                                    AS iam_labels,
        $get_md5(login)                                 AS login_hash
    FROM $prepared as a
    LEFT ONLY JOIN $dst_table as b
        ON a.iam_service_account_id = b.iam_service_account_id
        AND a.modified_ts = b.modified_ts
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_service_account_id,
    iam_service_account_status,
    iam_service_account_description_hash,
    created_ts,
    created_dttm_local,
    modified_ts,
    modified_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    iam_cloud_id,
    iam_folder_id,
    iam_labels,
    login_hash
FROM $result
ORDER BY
    iam_service_account_id;


INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT
    iam_service_account_id,
    iam_service_account_description,
    login
FROM
    $prepared
ORDER BY
    iam_service_account_id;
