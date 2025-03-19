PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_datetime_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_hour_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_month_name_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_quarter_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_half_year_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_year_by_timestamp;
IMPORT `helpers` SYMBOLS $get_md5;

$dm_yc_consumption = {{ param["dm_yc_consumption"] -> quote() }};
$destination_path = {{input1->table_quote()}};

$pre_agg = (
SELECT
  billing_account_usage_status,
  row_number() OVER w1 AS rn_any,
  if((billing_record_cost + billing_record_credit) != 0, row_number() OVER w2) AS rn_paid,
  billing_account_id,
  if((billing_record_cost + billing_record_credit) != 0, "billing_account_first_paid_", "billing_account_first_trial_")
    || sku_service_name ||"_consumption" AS event_type,
  COALESCE(sku_service_name,"UNKNOWN") AS sku_service_name,
  COALESCE(sku_service_name_eng,"UNKNOWN") AS sku_service_name_eng,
  billing_record_real_consumption AS billing_transaction_amount,
  billing_record_currency AS billing_transaction_currency,
  billing_record_msk_date AS msk_event_dt,
  billing_record_msk_half_year AS msk_event_half_year,
  billing_record_msk_month AS msk_event_month_name,
  billing_record_msk_quarter AS msk_event_quarter,
  billing_record_msk_year AS msk_event_year,
  "billing_account" AS event_entity_type,
  "billing" AS event_group,
  billing_account_id AS event_entity_id,
  sku_id
FROM $dm_yc_consumption
  WINDOW
    w1 AS (PARTITION BY billing_account_id ORDER BY billing_record_msk_date),
    w2 AS (PARTITION BY billing_account_id, if((billing_record_cost + billing_record_credit) != 0,  1 , 0)  ORDER BY  billing_record_msk_date)
);

$first_trial_service_consumption = (
SELECT
  billing_account_usage_status,
  billing_account_id,
  billing_transaction_amount,
  billing_transaction_currency,
  null AS billing_transaction_id,
  null AS billing_transaction_type,
  event_entity_id,
  event_entity_type,
  event_group,
  $get_md5(AsTuple(event_type, msk_event_dt, event_entity_id)) AS event_id,
  null AS event_timestamp,
  event_type,
  msk_event_dt,
  null AS  msk_event_dttm,
  msk_event_half_year,
  null AS msk_event_hour,
  msk_event_month_name,
  msk_event_quarter,
  msk_event_year,
  sku_id
FROM
    $pre_agg
  WHERE
    rn_any = 1
);

$first_paid_service_consumption = (
SELECT
  billing_account_usage_status,
  billing_account_id,
  billing_transaction_amount,
  billing_transaction_currency,
  null AS billing_transaction_id,
  null AS billing_transaction_type,
  event_entity_id,
  event_entity_type,
  event_group,
  $get_md5(AsTuple(event_type, msk_event_dt, event_entity_id)) AS event_id,
  null AS event_timestamp,
  event_type,
  msk_event_dt,
  null AS msk_event_dttm,
  msk_event_half_year,
  null AS msk_event_hour,
  msk_event_month_name,
  msk_event_quarter,
  msk_event_year,
  sku_id
FROM
    $pre_agg
  WHERE
    rn_paid = 1
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT * FROM $first_trial_service_consumption
UNION ALL
SELECT * FROM $first_paid_service_consumption
