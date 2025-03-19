PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};
$src_dir = {{ param["source_dir"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$str = ($field) -> (CAST($field AS String));

INSERT
    INTO $dst_table
    WITH TRUNCATE
SELECT
    $str(`id`)                              AS quota_request_limit_id,
    $str(`status`)                          AS status,
    $str(`processing_method`)               AS processing_method,
    `processing_details`                    AS processing_details,
    $str(`quota_id`)                        AS quota_id,
    $str(`quota_request_id`)                AS quota_request_id,
    $str(`message`)                         AS message,
    `approved_limit`                        AS approved_limit,
    `desired_limit`                         AS desired_limit,
    `current_limit`                         AS current_limit,
    `current_usage`                         AS current_usage
FROM
    $select_transfer_manager_table($src_dir, $cluster) AS quota_request_limits
ORDER BY quota_request_limit_id;
