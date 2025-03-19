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

$organizations_history = {{ param["organizations_history"] -> quote() }};
$destination_path = {{ input1 -> table_quote() }};

INSERT INTO $destination_path WITH TRUNCATE
SELECT
    null AS billing_account_id,
    null AS billing_transaction_amount,
    null AS billing_transaction_currency,
    null AS billing_transaction_id,
    null AS billing_transaction_type,
    organization_id AS event_entity_id,
    "cloud" AS event_entity_type,
    "iam" AS event_group,
    $get_md5(AsTuple(created_at_msk, organization_id)) AS event_id,
    cast(created_at_msk as UInt64) as event_timestamp,
    "create_organization" AS event_type,
    $format_msk_date_by_timestamp(created_at_msk) AS msk_event_dt,
    $format_msk_datetime_by_timestamp(created_at_msk) as msk_event_dttm,
    $format_msk_half_year_by_timestamp(created_at_msk) AS msk_event_half_year,
    $format_msk_hour_by_timestamp(created_at_msk) AS msk_event_hour,
    $format_msk_month_name_by_timestamp(created_at_msk) AS msk_event_month_name,
    $format_msk_quarter_by_timestamp(created_at_msk) AS msk_event_quarter,
    $format_msk_year_by_timestamp(created_at_msk) AS msk_event_year,
    null AS sku_id
FROM (
         SELECT t.*,
            ROW_NUMBER() OVER w AS rn
         FROM $organizations_history AS t
         WHERE status = "CREATING"
           AND created_at_msk IS NOT NULL
         WINDOW w AS (PARTITION BY organization_id ORDER BY modified_at_msk desc)
     )
WHERE rn = 1;
