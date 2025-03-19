PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_datetime_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_hour_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_month_name_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_quarter_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_half_year_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_year_by_timestamp;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $get_md5;
IMPORT `tables` SYMBOLS $get_all_daily_tables;

$cluster = {{cluster->table_quote()}};

$migration_timestamp = {{param["migration_timestamp"]}};
$billing_records_table = {{param["billing_records_table"]->quote()}};
$billing_accounts_table = {{param["billing_accounts_table"]->quote()}};
$realtime_metrics_dir = {{param["realtime_metrics_dir"]->quote()}};
$destination_path = {{input1->table_quote()}};

$ZERO = $to_decimal_35_9("0");

-- get billing accounts

$billing_accounts = (
  SELECT
    id AS billing_account_id,
    created_at,
  FROM $billing_accounts_table
);

-- get old billing records

$old_billing_records = (
  SELECT
    billing_account_id,
    sku_id,
    billing_records.start_time AS usage_start
  FROM $billing_records_table AS billing_records
  WHERE cost + credit = 0
);

-- get realtime metrics

$realtime_metrics_paths = (
  SELECT
    *
  FROM $get_all_daily_tables($realtime_metrics_dir, $cluster)
);

$realtime_billing_records = (
  SELECT
    billing_account_id,
    sku_id,
    Yson::LookupUint64(usage, "start") AS usage_start
  FROM EACH($realtime_metrics_paths)
  WHERE end_time > $migration_timestamp and $to_decimal_35_9(cost) + $to_decimal_35_9(credit) = $ZERO
);

$billing_records = (
  SELECT
    billing_account_id,
    MIN_BY(sku_id, usage_start) AS sku_id,
    MIN(usage_start) AS usage_start
  FROM(
    SELECT * FROM $old_billing_records
    UNION ALL
    SELECT * FROM $realtime_billing_records
  )
  GROUP BY billing_account_id
);

-- find first consumption

$result = (
  SELECT
    "billing_account_first_trial_consumption" AS event_type,
    MAX_OF(billing_accounts.created_at, records.usage_start) AS event_timestamp,
    billing_accounts.billing_account_id AS event_entity_id,
    billing_accounts.billing_account_id AS billing_account_id,
    records.sku_id AS sku_id,
  FROM $billing_accounts AS billing_accounts
  JOIN $billing_records AS records USING(billing_account_id)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  r.event_type as event_type,
  r.event_timestamp as event_timestamp,
  r.event_entity_id as event_entity_id,
  r.billing_account_id as billing_account_id,
  r.sku_id as sku_id,
  $get_md5(AsTuple(event_type, event_timestamp, event_entity_id)) AS event_id,
  "billing" as event_group,
  "billing_account" as event_entity_type,
  $format_msk_date_by_timestamp(event_timestamp) as msk_event_dt,
  $format_msk_datetime_by_timestamp(event_timestamp) as msk_event_dttm,
  $format_msk_hour_by_timestamp(event_timestamp) as msk_event_hour,
  $format_msk_month_name_by_timestamp(event_timestamp) as msk_event_month_name,
  $format_msk_quarter_by_timestamp(event_timestamp) as msk_event_quarter,
  $format_msk_half_year_by_timestamp(event_timestamp) as msk_event_half_year,
  $format_msk_year_by_timestamp(event_timestamp) as msk_event_year,
FROM $result as r
ORDER BY event_type, event_timestamp, event_entity_id;
