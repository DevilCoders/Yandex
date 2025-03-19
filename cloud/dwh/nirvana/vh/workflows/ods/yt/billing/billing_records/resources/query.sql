PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $concat_path;
IMPORT `tables` SYMBOLS $get_all_daily_tables_from_month;
IMPORT `tables` SYMBOLS $get_logfeller_monthly_tables_to_load;
IMPORT `numbers` SYMBOLS $to_decimal_35_15;
IMPORT `numbers` SYMBOLS $double_to_decimal_35_15;
IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_month_by_timestamp;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster -> table_quote() }};

$old_billing_records_table = {{ param["old_billing_records_table"] -> quote() }};
$enriched_metrics_folder = {{ param["enriched_metrics_folder"] -> quote() }};
$billing_records_corrections_table = {{ param["billing_records_corrections_table"] -> quote() }};
$destination_folder = {{ param["destination_folder"] -> quote() }};

$billing_start_date = {{ param["billing_start_date"] -> quote() }};
$migration_timestamp = {{ param["migration_timestamp"] }};
$reload_before_date = {{ param["reload_before_date"] -> quote() }};
$days_qty_to_reload =  {{ param["days_qty_to_reload"] }};
$limit_to_load = {{ param["limit_to_load"] }};

$migration_month = $format_msk_month_by_timestamp($migration_timestamp);
$limit_to_load = UNWRAP($limit_to_load);
$reload_before_datetime = CAST(date($reload_before_date) AS DateTime);

$get_month_from_date = ($dt) -> (SUBSTRING($dt, 0, 8) || '01');

$round = ($field) -> (Math::Round($field, -15));


-- get month to load
$tables_to_load = (
    SELECT *
    FROM $get_logfeller_monthly_tables_to_load($cluster, $enriched_metrics_folder, $destination_folder, $billing_start_date, $days_qty_to_reload, $reload_before_datetime, $limit_to_load)
);

-- show month to load
select * from $tables_to_load;

-- get old billing records
$old_billing_records = (
    SELECT
        billing_account_id                                                AS billing_account_id,
        NVL(cloud_id, '')                                                 AS cloud_id,
        ''                                                                AS folder_id,
        ''                                                                AS resource_id,
        $get_datetime(start_time)                                         AS start_time,
        $get_datetime(end_time)                                           AS end_time,
        $format_msk_date_by_timestamp(end_time)                           AS `date`,
        $format_msk_month_by_timestamp(end_time)                          AS month,
        sku_id                                                            AS sku_id,
        $double_to_decimal_35_15(pricing_quantity)                        AS pricing_quantity,
        $double_to_decimal_35_15(cost)                                    AS cost,
        $double_to_decimal_35_15(credit)                                  AS credit,
        $double_to_decimal_35_15(cost) + $double_to_decimal_35_15(credit) AS expense,
        $double_to_decimal_35_15(monetary_grant_credit)                   AS monetary_grant_credit,
        $double_to_decimal_35_15(committed_use_discount_credit)           AS cud_credit,
        $double_to_decimal_35_15(volume_incentive_credit)                 AS volume_incentive_credit,
        $double_to_decimal_35_15(trial_credit)                            AS trial_credit,
        $double_to_decimal_35_15(disabled_credit)                         AS disabled_credit,
        $double_to_decimal_35_15(service_credit)                          AS service_credit,
        labels_hash                                                       AS labels_hash,
    FROM $old_billing_records_table
);

DEFINE SUBQUERY $get_actual_billing_records($month) AS

    -- get paths to tables
    $enriched_metrics_paths = (
        SELECT
            paths
        FROM $get_all_daily_tables_from_month($enriched_metrics_folder, $cluster, $month)
    );

    -- get enriched metrics
    SELECT
        billing_account_id                                                                               AS billing_account_id,
        cloud_id                                                                                         AS cloud_id,
        folder_id                                                                                        AS folder_id,
        resource_id                                                                                      AS resource_id,
        $get_datetime(start_time_timestamp)                                                              AS start_time,
        $get_datetime(end_time_timestamp)                                                                AS end_time,
        $format_msk_date_by_timestamp(end_time)                                                          AS `date`,
        $format_msk_month_by_timestamp(end_time)                                                         AS month,
        SOME(currency)                                                                                   AS currency,
        schema                                                                                           AS metric_schema,
        sku_id                                                                                           AS sku_id,
        rate_id                                                                                          AS rate_id,
        pricing_unit                                                                                     AS pricing_unit,
        unit_price                                                                                       AS unit_price,
        sku_overridden                                                                                   AS sku_overridden,
        SUM($to_decimal_35_15(tiered_pricing_quantity))                                                  AS tiered_pricing_quantity,
        SUM($to_decimal_35_15(pricing_quantity))                                                         AS pricing_quantity,
        SUM($to_decimal_35_15(cost))                                                                     AS cost,
        SUM($to_decimal_35_15(credit))                                                                   AS credit,
        SUM($to_decimal_35_15(cost) + $to_decimal_35_15(credit))                                         AS expense,
        SUM($to_decimal_35_15(monetary_grant_credit))                                                    AS monetary_grant_credit,
        SUM($to_decimal_35_15(cud_credit))                                                               AS cud_credit,
        SUM($to_decimal_35_15(cud_compensated_pricing_quantity))                                         AS cud_compensated_pricing_quantity,
        SUM($to_decimal_35_15(volume_incentive_credit))                                                  AS volume_incentive_credit,
        SUM($to_decimal_35_15(trial_credit))                                                             AS trial_credit,
        SUM($to_decimal_35_15(disabled_credit))                                                          AS disabled_credit,
        SUM($to_decimal_35_15(service_credit))                                                           AS service_credit,

        SOME(master_account_id)                                                                          AS master_account_id,
        SUM($to_decimal_35_15("0"))                                                                      AS rewarded_expense,
        SUM($to_decimal_35_15(NVL(reward, volume_reward)))                                               AS reward,

        publisher_account_id                                                                             AS publisher_account_id,
        SOME(publisher_balance_client_id)                                                                AS publisher_balance_client_id,
        SOME(publisher_currency)                                                                         AS publisher_currency,
        SUM($to_decimal_35_15(revenue))                                                                  AS publisher_revenue,

        BITCAST(labels_hash AS Uint64)                                                                   AS labels_hash,
        NVL(SOME(labels_json), null)                                                                     AS labels_json,
        SUM($to_decimal_35_15(Yson::LookupString(usage, 'quantity',Yson::Options(true as AutoConvert)))) AS usage_quantity,

    FROM EACH($enriched_metrics_paths)
    WHERE end_time > $migration_timestamp AND $format_msk_month_by_timestamp(end_time) = $month
    GROUP BY billing_account_id,
        cloud_id,
        folder_id,
        resource_id,
        start_time as start_time_timestamp,
        end_time as end_time_timestamp,
        sku_id,
        schema,
        rate_id,
        pricing_unit,
        unit_price,
        sku_overridden,
        publisher_account_id,
        labels_hash

END DEFINE;

-- get billing records corrections
$billing_records_corrections = (
    SELECT
        billing_account_id                                                                          AS billing_account_id,
        NVL(cloud_id, '')                                                                           AS cloud_id,
        NVL(folder_id, '')                                                                          AS folder_id,
        NVL(resource_id, '')                                                                        AS resource_id,
        $get_datetime(Datetime::ToSeconds(Cast(`date` || 'T00:00:00,Europe/Moscow' as TzDatetime))) AS start_time,
        $get_datetime(Datetime::ToSeconds(Cast(`date` || 'T23:59:59,Europe/Moscow' as TzDatetime))) AS end_time,
        `date`                                                                                      AS `date`,
        $get_month_from_date(`date`)                                                                AS month,
        sku_id                                                                                      AS sku_id,
        $to_decimal_35_15('0')                                                                      AS pricing_quantity,
        $double_to_decimal_35_15($round(cost))                                                      AS cost,
        $double_to_decimal_35_15($round(credit))                                                    AS credit,
        $double_to_decimal_35_15($round(cost)) + $double_to_decimal_35_15($round(credit))           AS expense,

        labels_hash                                                                                 AS labels_hash,
    FROM $billing_records_corrections_table
);


DEFINE SUBQUERY $get_billing_records($month) AS
    $billing_records = (
        SELECT * FROM $get_actual_billing_records($month)
        UNION ALL
        SELECT * FROM $billing_records_corrections
    );

    SELECT * FROM $billing_records
END DEFINE;

DEFINE SUBQUERY $get_old_billing_records($month) AS
    $billing_records = (
        SELECT * FROM $old_billing_records
        UNION ALL
        SELECT * FROM $billing_records_corrections
    );

    SELECT * FROM $billing_records
END DEFINE;

DEFINE ACTION $load_old_billing_records($month) AS
    $destination_table = $concat_path($destination_folder, $month);
    INSERT INTO $destination_table WITH TRUNCATE
    SELECT *
    FROM $get_old_billing_records($month)
    WHERE month = $month
    ORDER BY end_time, billing_account_id;
    COMMIT;
END DEFINE;

DEFINE ACTION $load_billing_records($month) AS
    $destination_table = $concat_path($destination_folder, $month);
    INSERT INTO $destination_table WITH TRUNCATE
    SELECT *
    FROM $get_billing_records($month)
    WHERE month = $month
    ORDER BY end_time, billing_account_id;
    COMMIT;
END DEFINE;

-- if evaluation date is greater than migration date, then use only enriched metrics, otherwise use all records
DEFINE ACTION $run($month) AS
    EVALUATE IF $month <= $migration_month
        DO $load_old_billing_records($month)
    ELSE
        DO $load_billing_records($month);
END DEFINE;

-- evaluate for all dates
EVALUATE FOR $month IN $tables_to_load DO BEGIN
    DO $run($month)
END DO;
;
