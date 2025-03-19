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

$billing_accounts_table = {{param["billing_accounts_table"]->quote()}};
$destination_path = {{input1->table_quote()}};

$billing_accounts = (
  SELECT
    AsList(
      AsStruct(
        "billing_account_created" AS event_type,
        created_at AS event_timestamp,
        id AS event_entity_id,
        id AS billing_account_id,
      ),
      AsStruct(
        "billing_account_became_paid" AS event_type,
        paid_at AS event_timestamp,
        id AS event_entity_id,
        id AS billing_account_id,
      ),
      AsStruct(
        "billing_account_became_disabled" AS event_type,
        disabled_at AS event_timestamp,
        id AS event_entity_id,
        id AS billing_account_id,
      ),
    ) as x
  FROM $billing_accounts_table
);

$flatten_structed_events = (
  SELECT
    *
  FROM $billing_accounts
  FLATTEN LIST BY x
);

$result = (
  SELECT
    *
  FROM $flatten_structed_events
  FLATTEN COLUMNS
  WHERE event_timestamp IS NOT NULL
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
