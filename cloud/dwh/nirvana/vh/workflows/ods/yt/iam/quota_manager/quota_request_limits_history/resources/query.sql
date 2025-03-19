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
        `processing_method`         AS processing_method,
        `processing_details`        AS processing_details,
        `approved_limit`            AS approved_limit,
        `deleted_at`                AS deleted_at,
        `desired_limit`             AS desired_limit,
        `message`                   AS message,
        `quota_id`                  AS quota_id,
        `quota_request_id`          AS quota_request_id,
        `current_limit`             AS current_limit,
        `current_usage`             AS current_usage,
        CAST(`metadata` AS JSON)    AS metadata_json,
    FROM
        $select_transfer_manager_table($src_dir, $cluster) AS quota_requests
);

INSERT
    INTO $dst_table
    WITH TRUNCATE
SELECT
    $str(`id`)                                          AS quota_request_limit_id,
    $str(`status`)                                      AS status,
    $str(`processing_method`)                           AS processing_method,
    `processing_details`                                AS processing_details,
    $str(`quota_id`)                                    AS quota_id,
    $str(`quota_request_id`)                            AS quota_request_id,
    $str(`message`)                                     AS message,
    `approved_limit`                                    AS approved_limit,
    `desired_limit`                                     AS desired_limit,
    `current_limit`                                     AS current_limit,
    `current_usage`                                     AS current_usage,
    metadata_json                                       AS metadata,
    $str(JSON_VALUE(`metadata_json`, "$.tx_id"))        AS metadata_tx_id,
    `modified_at`                                       AS modified_ts,
    $from_utc_ts_to_msk_dt(`modified_at`)               AS modified_dttm_local,
    `deleted_at`                                        AS deleted_ts,
    $from_utc_ts_to_msk_dt(`deleted_at`)                AS deleted_dttm_local,
    "MSK"                                               AS dttm_tz,
FROM
    $quota_requests_parsed
ORDER BY quota_request_id, modified_ts;


