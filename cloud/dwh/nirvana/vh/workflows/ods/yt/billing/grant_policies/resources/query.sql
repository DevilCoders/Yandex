PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    `aggregation_span`                  AS aggregation_span,
    $to_ts(`created_at`)                AS created_ts,
    $to_dttm_local(`created_at`)        AS created_dttm_local,
    $to_str(`currency`)                 AS currency,
    $to_ts(`deleted_at`)                AS deleted_ts,
    $to_dttm_local(`deleted_at`)        AS deleted_dttm_local,
    $to_ts(`end_time`)                  AS end_ts,
    $to_dttm_local(`end_time`)          AS end_dttm_local,
    `grant_duration`                    AS grant_duration,
    $to_str(`id`)                       AS grant_policy_id,
    $to_str(`name`)                     AS grant_policy_name,
    $to_decimal_35_9(`max_amount`)      AS max_amount,
    `max_count`                         AS max_count,
    $to_ts(`paid_end_time`)             AS paid_end_ts,
    $to_dttm_local(`paid_end_time`)     AS paid_end_dttm_local,
    $to_ts(`paid_start_time`)           AS paid_start_ts,
    $to_dttm_local(`paid_start_time`)   AS paid_start_dttm_local,
    $to_decimal_35_9(`rate`)            AS rate,
    $to_ts(`start_time`)                AS start_ts,
    $to_dttm_local(`start_time`)        AS start_dttm_local,
    $to_str(`state`)                    AS state
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY grant_policy_id
