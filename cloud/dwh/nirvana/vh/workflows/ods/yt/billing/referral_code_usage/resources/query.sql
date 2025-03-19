PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `helpers` SYMBOLS $to_str, $yson_to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_ts(CAST($yson_to_str(`activated_at`) AS Uint64))            AS activated_ts,
    $to_dttm_local(CAST($yson_to_str(`activated_at`) AS Uint64))    AS activated_dttm_local,
    $to_ts(`created_at`)                                            AS created_ts,
    $to_dttm_local(`created_at`)                                    AS created_dttm_local,
    $to_str(`referral_code_id`)                                     AS referral_code_id,
    $to_str(`referral_id`)                                          AS referral_id,
    $to_str(`referrer_id`)                                          AS referrer_id
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY `referral_id`,`referrer_id`
