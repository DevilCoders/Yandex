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

$get_uint64_from_dicts= ($container, $key_lev1, $key_lev2, $default) -> (cast(Yson::ConvertToString(DictLookup(Yson::LookupDict($container, $key_lev1, $strict_options), $key_lev2), $autoconvert_options) as Uint64));

INSERT INTO $dst_table WITH TRUNCATE
SELECT a.* ,
    $lookup_string(tiered_billed_schemas,'schema', NULL)                        AS tiered_billed_schema_schema,
    cast($lookup_string(tiered_billed_schemas,'timespan', NULL) AS Uint64)      AS tiered_billed_schema_timespan
WITHOUT
    tiered_billed_schemas
FROM (
    SELECT
        $to_str(id)                                                             AS committed_use_discount_template_id,
        $to_str(compensated_sku_id)                                             AS compensated_sku_id,
        $get_datetime(created_at)                                               AS created_ts,
        $from_utc_ts_to_msk_dt($get_datetime(created_at))                       AS create_dttm_local,
        $to_str(purchase_unit)                                                  AS purchase_unit,
        Yson::ConvertToList(tiered_billed_schemas,$autoconvert_options)         AS tiered_billed_schemas,
        $get_uint64_from_dicts(constraints, "purchase_quantity", "min", NULL)   AS constraint_purchase_quantity_min,
        $get_uint64_from_dicts(constraints, "purchase_quantity", "max", NULL)   AS constraint_purchase_quantity_max,
        $get_uint64_from_dicts(constraints, "purchase_quantity", "step", NULL)  AS constraint_purchase_quantity_step,
        $get_uint64_from_dicts(constraints, "timespan", "min", NULL)            AS constraint_timespan_quantity_min,
        $get_uint64_from_dicts(constraints, "timespan", "max", NULL)            AS constraint_timespan_quantity_max,
        $get_uint64_from_dicts(constraints, "timespan", "step", NULL)           AS constraint_timespan_quantity_step,
        is_private                                                              AS is_private,
        order                                                                   AS order,
        $get_datetime(updated_at)                                               AS updated_ts,
        $from_utc_ts_to_msk_dt($get_datetime(updated_at))                       AS updated_dttm_local
    FROM $select_transfer_manager_table($src_folder, $cluster)
) AS a
FLATTEN LIST BY tiered_billed_schemas
ORDER BY committed_use_discount_template_id
