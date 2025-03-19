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

-- (Parameter) Таблица истории изменения биллинг аккаунтов
$billing_accounts_history_table = {{ param["billing_accounts_history_table"]->quote() }};
-- (Parameter) Выходная таблица
$destination_path = {{ input1->table_quote() }};


$get_ts_from_datetime = ($dttm) -> (Datetime::ToSeconds($dttm));


$result = (
  SELECT
    "billing_account_became_trial" as event_type,
    $get_ts_from_datetime(min(updated_at)) as event_timestamp,
    billing_account_id as event_entity_id,
    billing_account_id
  FROM $billing_accounts_history_table
  where usage_status='trial'
  group by billing_account_id
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  event_type,
  event_timestamp,
  event_entity_id,
  billing_account_id,
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
FROM $result
ORDER BY event_type, event_timestamp, event_entity_id;
