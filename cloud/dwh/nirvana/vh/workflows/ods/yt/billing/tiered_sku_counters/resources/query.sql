PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;

$cluster = {{cluster->table_quote()}};

$src_folder = {{ param["source_folder_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};
$to_str = ($data) -> (CAST($data AS String));

$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

$result = (
SELECT
    $to_str(`billing_account_id`)                               AS billing_account_id,
    $to_str(`interval`)                                         AS `interval`,
    $to_decimal_35_9(`pricing_quantity`)                        AS pricing_quantity,
    `shard_id`                                                  AS shard_id,
    $to_str(`sku_id`)                                           AS sku_id,
    $get_datetime(`updated_at`)                                 AS updated_ts,
    $from_utc_ts_to_msk_dt($get_datetime(`updated_at`))         AS updated_dttm_local
FROM $snapshot
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY billing_account_id, `interval`, shard_id, sku_id;
