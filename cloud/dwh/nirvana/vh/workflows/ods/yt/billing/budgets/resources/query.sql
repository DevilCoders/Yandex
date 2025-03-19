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

$to_ts = ($container)  -> ($get_datetime(cast($yson_to_str($container) as Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($yson_to_str($container) as Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
*
WITHOUT
    threshold_rule,
FROM (
    SELECT
        a.*,
        filter_info.cloud_and_folder_ids                            AS filter_info_cloud_and_folder_ids,
        threshold_rule.threshold                                    AS threshold_rule_threshold,
        threshold_rule.threshold_amount                             AS threshold_rule_threshold_amount,
        threshold_rule.threshold_type                               AS threshold_rule_threshold_type,
    WITHOUT
        threshold_rules,
        filter_info,
        notification_users
        FROM (
        SELECT
            $to_str(billing_account_id)                             AS billing_account_id,
            $to_str(budget_type)                                    AS budget_type,
            $to_decimal_35_9(budgeted_amount)                       AS budgeted_amount,
            $to_ts(created_at)                                      AS created_ts,
            $to_dttm_local(created_at)                              AS created_dttm_local,
            $to_str(created_by)                                     AS created_by,
            $to_ts(expiration_date)                                 AS end_date_ts,
            $to_dttm_local(expiration_date)                         AS end_date_dttm_local,
            Yson::ConvertTo(filter_info,
                    Struct<
                        cloud_and_folder_ids: String?, service_ids: List<String?>?
                    >?
                    )                                               AS filter_info,
            $to_str(id)                                             AS budget_id,
            $to_str(name)                                           AS budget_name,
            Yson::ConvertTo(notification_users, List<String?>? )    AS notification_users,
            $to_str(reset_period)                                   AS reset_period,
            $to_ts(start_date)                                      AS start_date_ts,
            $to_dttm_local(start_date)                              AS start_date_dttm_local,
            $to_str(state)                                          AS state,
            Yson::ConvertTo(threshold_rules,
                    List<
                        Struct<notification_users: List<String?>?, threshold: String?, threshold_amount: String?, threshold_type: String?>?
                    >?
                    )                                               AS threshold_rules,
            $to_ts(updated_at)                                      AS updated_ts,
            $to_dttm_local(updated_at)                              AS updated_dttm_local,
            use_in_cloud_function                                   AS use_in_cloud_function
        FROM $select_transfer_manager_table($src_folder, $cluster)
        ) AS a
    FLATTEN LIST BY (threshold_rules AS threshold_rule, filter_info.service_ids AS filter_info_service_id, notification_users AS notification_user)
) AS a
FLATTEN LIST BY threshold_rule.notification_users AS threshold_rule_notification_users
order by billing_account_id, budget_id
