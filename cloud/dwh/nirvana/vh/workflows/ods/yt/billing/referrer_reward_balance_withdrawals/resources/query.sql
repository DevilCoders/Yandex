PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $to_str, $yson_to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_decimal_35_9(`amount`)                                  AS amount,
    $to_str(`balance_client_id`)                                AS balance_client_id,
    $to_ts(CAST($yson_to_str(`created_at`) AS Uint64))          AS created_ts,
    $to_dttm_local(CAST($yson_to_str(`created_at`) AS Uint64))  AS created_dttm_local,
    $to_str(`withdrawal_id`)                                    AS withdrawal_id
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY withdrawal_id
