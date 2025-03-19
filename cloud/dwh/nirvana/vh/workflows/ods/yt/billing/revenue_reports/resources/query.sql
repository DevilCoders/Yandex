PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT tables SYMBOLS $select_transfer_manager_table;
IMPORT datetime SYMBOLS $get_datetime, $get_date, $from_utc_ts_to_msk_dt;
IMPORT numbers SYMBOLS $to_decimal_35_9;
IMPORT helpers SYMBOLS $to_str, $lookup_bool, $lookup_string_list, $lookup_int64, $lookup_string, $autoconvert_options, $yson_to_uint64, $yson_to_str, $yson_to_list, $get_json_from_yson;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_str(`billing_account_id`)           AS billing_account_id,
    $to_ts($to_str(`created_at`))           AS created_ts,
    $to_dttm_local($to_str(`created_at`))   AS created_dttm_local,
    $get_date($to_str(`date`))              AS `date`,
    $to_str(`publisher_account_id`)         AS publisher_account_id,
    $to_str(`publisher_balance_client_id`)  AS publisher_balance_client_id,
    $to_str(`publisher_currency`)           AS publisher_currency,
    $to_str(`sku_id`)                       AS sku_id,
    $to_decimal_35_9(`total`)               AS total
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY `publisher_account_id`, `date`, `sku_id`, `billing_account_id`
