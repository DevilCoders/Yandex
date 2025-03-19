PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $lookup_bool, $lookup_string_list, $lookup_int64, $lookup_string, $autoconvert_options, $strict_options, $to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_str(billing_account_id)                         AS billing_account_id,
    $get_datetime(end_time)                             AS end_ts,
    $from_utc_ts_to_msk_dt($get_datetime(end_time))     AS end_dttm_local,
    $to_str(id)                                         AS committed_use_discount_id,
    $to_str(compensated_sku_id)                         AS compensated_sku_id,
    $get_datetime(created_at)                           AS created_ts,
    $from_utc_ts_to_msk_dt($get_datetime(created_at))   AS created_dttm_local,
    $to_decimal_35_9(pricing_quantity)                  AS pricing_quantity,
    $to_str(pricing_unit)                               AS pricing_unit,
    $get_datetime(start_time)                           AS start_ts,
    $from_utc_ts_to_msk_dt($get_datetime(start_time))   AS start_local
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY billing_account_id, end_ts, committed_use_discount_id
