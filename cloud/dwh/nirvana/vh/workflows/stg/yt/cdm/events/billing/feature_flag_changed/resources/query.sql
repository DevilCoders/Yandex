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

$get_event_type = ($name, $value) -> ("billing_account_" || IF($value, "became", "no_longer") || "_" || $name);

$history = (
  SELECT
    updated_at as event_timestamp,
    billing_account_id as event_entity_id,
    billing_account_id,
    is_isv as current_is_isv,
    LAG(is_isv) over w as previous_is_isv,
    is_var as current_is_var,
    LAG(is_var) over w as previous_is_var,
    is_referral as current_is_referral,
    LAG(is_referral) over w as previous_is_referral,
    is_referrer as current_is_referrer,
    LAG(is_referrer) over w as previous_is_referrer,
  FROM $billing_accounts_history_table
  WINDOW w AS (
    PARTITION COMPACT BY billing_account_id ORDER BY updated_at
    ROWS BETWEEN 1 PRECEDING AND CURRENT ROW
  )
);

$result = (
  SELECT
    $get_event_type("isv", current_is_isv) as event_type,
    event_timestamp,
    event_entity_id,
    billing_account_id
  FROM $history
  WHERE current_is_isv != previous_is_isv OR (previous_is_isv IS NULL AND current_is_isv)

  UNION ALL

  SELECT
    $get_event_type("var", current_is_var) as event_type,
    event_timestamp,
    event_entity_id,
    billing_account_id
  FROM $history
  WHERE current_is_var != previous_is_var OR (previous_is_var IS NULL AND current_is_var)

  UNION ALL

  SELECT
    $get_event_type("referral", current_is_referral) as event_type,
    event_timestamp,
    event_entity_id,
    billing_account_id
  FROM $history
  WHERE current_is_referral != previous_is_referral OR (previous_is_referral IS NULL AND current_is_referral)

  UNION ALL

  SELECT
    $get_event_type("referrer", current_is_referrer) as event_type,
    event_timestamp,
    event_entity_id,
    billing_account_id
  FROM $history
  WHERE current_is_referrer != previous_is_referrer OR (previous_is_referrer IS NULL AND current_is_referrer)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  r.event_type as event_type,
  r.event_timestamp as event_timestamp,
  r.event_entity_id as event_entity_id,
  r.billing_account_id as billing_account_id,
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
