PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_date, $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $to_str;


$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_str(`balance_product_id`)               AS balance_product_id,
    $to_str(`billing_account_id`)               AS billing_account_id,
    $to_ts(`created_at`)                        AS created_ts,
    $to_dttm_local(`created_at`)                AS created_dttm_local,
    $get_date($to_str(`date`))                  AS `date`,
    $to_ts(`subaccount_id`)                     AS subaccount_id,
    $to_decimal_35_9(`total`)                   AS total
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY `billing_account_id`,`balance_product_id`,`subaccount_id`,`date`
