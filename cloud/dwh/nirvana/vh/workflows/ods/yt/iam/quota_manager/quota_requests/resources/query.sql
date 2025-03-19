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
    $str(`id`)                              AS quota_request_id,
    $str(`status`)                          AS status,
    $str(`resource_type`)                   AS resource_type,
    $str(`issue_id`)                        AS issue_id,
    $str(`resource_id`)                     AS resource_id,
    $str(`billing_account_id`)              AS billing_account_id,
    `verification_score`                    AS verification_score,
    `created_at`                            AS create_ts,
    $from_utc_ts_to_msk_dt(`created_at`)    AS create_dttm_local,
    "MSK"                                   AS dttm_tz
FROM
    $select_transfer_manager_table($src_dir, $cluster) AS quota_requests
ORDER BY quota_request_id;
