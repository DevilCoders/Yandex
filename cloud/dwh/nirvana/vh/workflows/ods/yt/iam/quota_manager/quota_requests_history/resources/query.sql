PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};
$src_dir = {{ param["source_dir"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$str = ($field) -> (CAST($field AS String));

$quota_requests_parsed = (
    SELECT
        `modified_at`               AS modified_at,
        `id`                        AS id,
        `metadata`                  AS metadata,
        `status`                    AS status,
        `created_at`                AS created_at,
        `deleted_at`                AS deleted_at,
        `issue_id`                  AS issue_id,
        `resource_id`               AS resource_id,
        `resource_type`             AS resource_type,
        `billing_account_id`        AS billing_account_id,
        `verification_score`        AS verification_score,
        CAST(`metadata` AS JSON)    AS metadata_json,
FROM
    $select_transfer_manager_table($src_dir, $cluster) AS quota_requests
);

INSERT
    INTO $dst_table
    WITH TRUNCATE
SELECT
    $str(`id`)                                          AS quota_request_id,
    $str(`status`)                                      AS status,
    $str(`resource_type`)                               AS resource_type,
    $str(`issue_id`)                                    AS issue_id,
    $str(`resource_id`)                                 AS resource_id,
    $str(`billing_account_id`)                          AS billing_account_id,
    `verification_score`                                AS verification_score,
    metadata_json                                       AS metadata,
    $str(JSON_VALUE(`metadata_json`, "$.request_id"))   AS metadata_request_id,
    $str(JSON_VALUE(`metadata_json`, "$.tx_id"))        AS metadata_tx_id,
    $str(JSON_VALUE(`metadata_json`, "$.user_id"))      AS metadata_user_id,
    `created_at`                                        AS created_ts,
    $from_utc_ts_to_msk_dt(`created_at`)                AS created_dttm_local,
    `modified_at`                                       AS modified_ts,
    $from_utc_ts_to_msk_dt(`modified_at`)               AS modified_dttm_local,
    `deleted_at`                                        AS deleted_ts,
    $from_utc_ts_to_msk_dt(`deleted_at`)                AS deleted_dttm_local,
    "MSK"                                               AS dttm_tz,
FROM
    $quota_requests_parsed
ORDER BY quota_request_id, modified_ts;


