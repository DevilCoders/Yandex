PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $lookup_bool, $lookup_string_list, $lookup_int64, $lookup_string, $autoconvert_options, $to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    a.*,
    $lookup_string(thresholds,'multiplier', NULL)   AS thresholds_multiplier,
    $lookup_string(thresholds,'start_amount', NULL) AS thresholds_start_amount,
WITHOUT
    thresholds
FROM (
    SELECT
        aggregation_info                                            AS aggregation_info,
        $to_str(billing_account_id)                                 AS billing_account_id,
        $get_datetime(created_at)                                   AS created_ts,
        $from_utc_ts_to_msk_dt($get_datetime(created_at))           AS created_dttm_local,
        $get_datetime(effective_time)                               AS start_ts,
        $from_utc_ts_to_msk_dt($get_datetime(effective_time))       AS start_dttm_local,
        $get_datetime(expiration_time)                              AS end_ts,
        $from_utc_ts_to_msk_dt($get_datetime(expiration_time))      AS end_local,
        $lookup_string(filter_info,'category', NULL)                AS filter_info_category,
        $lookup_string_list(filter_info,'entity_ids')               AS filter_info_entity_ids,
        $lookup_string(filter_info,'level', NULL)                   AS filter_info_level,
        $to_str(id)                                                 AS volume_incentive_id,
        Yson::ConvertToList(thresholds,$autoconvert_options)        AS thresholds
FROM $select_transfer_manager_table($src_folder, $cluster)
) AS a
FLATTEN LIST BY thresholds
ORDER BY billing_account_id, start_ts, volume_incentive_id
