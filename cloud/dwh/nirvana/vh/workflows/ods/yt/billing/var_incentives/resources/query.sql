PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `helpers` SYMBOLS $to_str, $lookup_string, $autoconvert_options, $yson_to_uint64;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    a.*
WITHOUT
    filter_info,
    filters,
    default_usage_intervals,
    reward_thresholds,
    subaccount_binding_time,
    subaccount_bindings
from (
    Select
        a.*,
        filter_info.category                                                                AS filter_info_category,
        filter_info.entity_ids ?? [null]                                                    AS filter_info_entity_id,
        filter_info.level                                                                   AS filter_info_level,
        filters.category                                                                    AS filter_category,
        filters.entity_ids ?? [null]                                                        AS filter_entity_id,
        filters.level                                                                       AS filter_level,
        $get_datetime(default_usage_intervals.effective_time)                               AS default_usage_interval_start_ts,
        $from_utc_ts_to_msk_dt($get_datetime(default_usage_intervals.effective_time))       AS default_usage_interval_start_dttm_local,
        $get_datetime(default_usage_intervals.expiration_time)                              AS default_usage_interval_expiration_time_ts,
        $from_utc_ts_to_msk_dt($get_datetime(default_usage_intervals.expiration_time))      AS default_usage_interval_expiration_time_dttm_local,
        reward_thresholds.multiplier                                                        AS reward_threshold_multiplier,
        reward_thresholds.start_amount                                                      AS reward_threshold_start_amount,
        $get_datetime(subaccount_binding_time.effective_time)                               AS subaccount_binding_start_ts,
        $from_utc_ts_to_msk_dt($get_datetime(subaccount_binding_time.effective_time))       AS subaccount_binding_start_dttm_local,
        $get_datetime(subaccount_binding_time.expiration_time)                              AS subaccount_binding_expiration_time_ts,
        $from_utc_ts_to_msk_dt($get_datetime(subaccount_binding_time.expiration_time))      AS subaccount_binding_expiration_time_dttm_local
    from
    (
        SELECT
            a.*,
            subaccount_bindings.0                                                           AS subaccount_binding_id,
            subaccount_bindings.1                                                           AS subaccount_binding_time
    FROM
        (
        SELECT
            $lookup_string(aggregation_info,'interval', NULL)                               AS aggregation_info_interval,
            $lookup_string(aggregation_info,'level', NULL)                                  AS aggregation_info_level,
            $get_datetime(created_at)                                                       AS created_ts,
            $from_utc_ts_to_msk_dt($get_datetime(created_at))                               AS created_dttm_local,
            $get_datetime($yson_to_uint64(default_subaccount_bindings_effective_time))      AS default_subaccount_bindings_start_ts,
            $from_utc_ts_to_msk_dt($get_datetime($yson_to_uint64(default_subaccount_bindings_effective_time)))
                                                                                            AS default_subaccount_bindings_start_dttm_local,
            $get_datetime($yson_to_uint64(default_subaccount_bindings_expiration_time))     AS default_subaccount_bindings_end_ts,
            $from_utc_ts_to_msk_dt($get_datetime($yson_to_uint64(default_subaccount_bindings_expiration_time)))
                                                                                            AS default_subaccount_bindings_end_dttm_local,
            Yson::ConvertTo(default_usage_intervals,
            List<
                Struct<effective_time: String?, expiration_time: String? >?
            >?, $autoconvert_options
            ) ??  [null]                                                                    AS default_usage_intervals,
            $get_datetime(effective_time)                                                   AS start_ts,
            $from_utc_ts_to_msk_dt($get_datetime(effective_time))                           AS start_dttm_local,
            $get_datetime(expiration_time)                                                  AS end_ts,
            $from_utc_ts_to_msk_dt($get_datetime(expiration_time))                          AS end_dttm_local,
            -- filter_info,
            Yson::ConvertTo(filter_info,
                Struct<category: String?, entity_ids: List<String?>?, level: String?>?
                , $autoconvert_options
            )                                                                               AS filter_info,
            Yson::ConvertTo(filters,
            List<
                Struct<category: String?, entity_ids: List<String?>?, level: String?>?
            >?, $autoconvert_options
            ) ?? [null]                                                                     AS filters,
            $to_str(id)                                                                     AS var_incentive_id,
            $to_str(master_account_id)                                                      AS master_account_id,
            Yson::ConvertTo(reward_thresholds,
            List<
                Struct<multiplier: String?, start_amount: String? >?
            >?, $autoconvert_options
            ) ?? [null]                                                                     AS reward_thresholds,
            $to_str(source_id)                                                              AS source_id,
            $to_str(source_type)                                                            AS source_type,
            NVL(Yson::ConvertTo(subaccount_bindings,
            Dict<String,
                    List <
                        Struct<effective_time: String?, expiration_time: String? >?
                        >?
            >?, $autoconvert_options
            ),{cast(null as String?):cast([null] as List<Struct<effective_time: String?, expiration_time: String? >?>?)})
                                                                                            AS subaccount_bindings
        FROM $select_transfer_manager_table($src_folder, $cluster)
        ) AS a
        FLATTEN DICT BY subaccount_bindings
    ) AS a
    FLATTEN LIST BY (reward_thresholds, filters, default_usage_intervals,subaccount_binding_time)
) as a
FLATTEN LIST BY (filter_entity_id, filter_info_entity_id)
order by start_ts,var_incentive_id,master_account_id
