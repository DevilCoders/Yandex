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

$to_ts = ($container)  -> ($get_datetime(cast($yson_to_str($container) AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($yson_to_str($container) AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_str(`act_month`)                    AS act_month,
    $to_str(`referral_id`)                  AS referral_id,
    $to_str(`referrer_id`)                  AS referrer_id,
    $to_decimal_35_9(`reward`)              AS reward,
    $to_str(`reward_month`)                 AS reward_month,
    $to_ts(`updated_at`)                    AS updated_ts,
    $to_dttm_local(`updated_at`)            AS updated_dttm_local
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY act_month, referral_id, referrer_id, reward_month
