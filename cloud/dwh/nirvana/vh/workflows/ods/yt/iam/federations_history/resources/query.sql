PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt, $get_datetime_secs;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$str = ($field) -> (CAST($field AS String));

$federations_hist_primary_processed = (
SELECT
    id                                              AS id,
    $get_datetime_secs(CAST(a.modified_at AS uint32)) AS modified_ts,
    $get_datetime_secs(CAST(a.created_at AS uint32))  AS created_ts,
    $get_datetime_secs(CAST(a.deleted_at AS uint32))  AS deleted_ts,
    a.description                                     AS description,
    a.folder_id                                       AS folder_id,
    a.issuer                                          AS issuer,
    a.name                                            AS name,
    a.sso_binding                                     AS sso_binding,
    a.sso_url                                         AS sso_url,
    a.cookie_max_age                                  AS cookie_max_age,
    a.autocreate_users                                AS autocreate_users,
    a.encrypted_assertions                            AS encrypted_assertions,
    a.status                                          AS status,
    a.case_insensitive_name_id                        AS case_insensitive_name_id,
    a.case_insensitive_name_ids                       AS case_insensitive_name_ids,
    a.set_sso_url_username                            AS set_sso_url_username,
    cast(a.metadata as JSON)                          AS metadata_json
FROM $select_transfer_manager_table($src_folder, $cluster) as a
LEFT JOIN $dst_table as b
ON a.id=b.iam_federation_id AND $get_datetime_secs(CAST(a.modified_at AS uint32))=b.modified_ts
WHERE b.iam_federation_id IS NULL
);

$result = (
SELECT
    iam_federation_id,
    modified_ts,
    modified_dttm_local,
    created_ts,
    created_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    description,
    iam_folder_id,
    issuer,
    name,
    sso_binding,
    sso_url,
    cookie_max_age,
    autocreate_users,
    encrypted_assertions,
    status,
    case_insensitive_name_id,
    case_insensitive_name_ids,
    set_sso_url_username,
    metadata_user_id
FROM $dst_table
UNION ALL
SELECT
    $str(id)                                            AS iam_federation_id,
    modified_ts                                         AS modified_ts,
    $from_utc_ts_to_msk_dt(modified_ts)                 AS modified_dttm_local,
    created_ts                                          AS created_ts,
    $from_utc_ts_to_msk_dt(created_ts)                  AS created_dttm_local,
    deleted_ts                                          AS deleted_ts,
    $from_utc_ts_to_msk_dt(deleted_ts)                  AS deleted_dttm_local,
    $str(description)                                   AS description,
    $str(folder_id)                                     AS iam_folder_id,
    $str(issuer)                                        AS issuer,
    $str(name)                                          AS name,
    $str(sso_binding)                                   AS sso_binding,
    $str(sso_url)                                       AS sso_url,
    cookie_max_age                                      AS cookie_max_age,
    autocreate_users                                    AS autocreate_users,
    encrypted_assertions                                AS encrypted_assertions,
    $str(status)                                        AS status,
    case_insensitive_name_id                            AS case_insensitive_name_id,
    case_insensitive_name_ids                           AS case_insensitive_name_ids,
    set_sso_url_username                                AS set_sso_url_username,
    $str(JSON_VALUE(`metadata_json`, "$.user_id"))      AS metadata_user_id,
FROM
    $federations_hist_primary_processed
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_federation_id,
    modified_ts,
    modified_dttm_local,
    created_ts,
    created_dttm_local,
    deleted_ts,
    deleted_dttm_local,
    description,
    iam_folder_id,
    issuer,
    name,
    sso_binding,
    sso_url,
    cookie_max_age,
    autocreate_users,
    encrypted_assertions,
    status,
    case_insensitive_name_id,
    case_insensitive_name_ids,
    set_sso_url_username,
    metadata_user_id
FROM
    $result
ORDER BY
    iam_federation_id, modified_ts;
