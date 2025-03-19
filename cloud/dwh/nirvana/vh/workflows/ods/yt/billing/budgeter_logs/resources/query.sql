PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $to_str, $yson_to_str, $autoconvert_options;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    a.*,
    periods.0                                           AS period_id,
    $to_decimal_35_9((periods.1).amount)                AS period_amount,
    $to_decimal_35_9((periods.1).execution_time)        AS period_execution_time,
    cast((periods.1).row_count AS Uint64)               AS period_row_count,
    $to_ts((periods.1).updated_at)                      AS period_updated_ts,
    $to_dttm_local((periods.1).updated_at)              AS period_updated_dttm_local
WITHOUT
    periods
FROM (
    SELECT
        $to_str(`billing_account_id`)                AS billing_account_id,
        $to_str(`budget_id`)                         AS budget_id,
        NVL(Yson::ConvertTo(periods,
            Dict<String,
                    Struct<amount: String?, execution_time: String? , row_count: String?, updated_at: String?>?
        >?, $autoconvert_options
        ), {cast(null as String?): cast(Null as Struct<amount: String?, execution_time: String? , row_count: String?, updated_at: String?>?)
        })                                          AS periods,
        `shard_id`                                  AS shard_id,
        $to_str(`state`)                            AS state,
        $to_ts($yson_to_str(`updated_at`))          AS updated_ts,
        $to_dttm_local($yson_to_str(`updated_at`))  AS updated_dttm_local
    FROM $select_transfer_manager_table($src_folder, $cluster)
) AS a
FLATTEN DICT BY periods
ORDER BY billing_account_id, budget_id, shard_id
