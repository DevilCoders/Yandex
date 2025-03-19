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
    $to_str(id)                                         AS subscription_id,
    $to_str(owner_id)                                   AS owner_id,
    $to_str(owner_type)                                 AS owner_type,
    $get_datetime(created_at)                           AS created_ts,
    $from_utc_ts_to_msk_dt($get_datetime(created_at))   AS created_dttm_local,
    $to_str(`schema`)                                   AS `schema`,
    $get_datetime(start_time)                           AS start_ts,
    $from_utc_ts_to_msk_dt($get_datetime(start_time))   AS start_dttm_local,
    $get_datetime(end_time)                             AS end_ts,
    $from_utc_ts_to_msk_dt($get_datetime(end_time))     AS end_dttm_local,
    $to_str(template_id)                                AS template_id,
    $to_str(template_type)                              AS template_type,
    $get_datetime(updated_at)                           AS updated_ts,
    $from_utc_ts_to_msk_dt($get_datetime(updated_at))   AS updated_dttm_local,
    purchase_unit                                       AS purchase_unit,
    $to_str(usage_unit)                                 AS usage_unit,
    purchase_quantity                                   AS purchase_quantity,
    Yson::ConvertToString(labels, Yson::Options(false as Strict))
                                                        AS labels,
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY subscription_id, owner_id,owner_type
