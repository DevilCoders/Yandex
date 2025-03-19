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

$clouds_table = {{ param["clouds_table"] -> quote() }};
$service_instance_bindings_table = {{ param["service_instance_bindings_table"] -> quote() }};
$destination_path = {{ input1 -> table_quote() }};

$cloud_ba = (
    SELECT
        service_instance_id, 
        billing_account_id,
        ROW_NUMBER() OVER(PARTITION BY service_instance_id ORDER BY created_at ASC) AS bind_order
    FROM $service_instance_bindings_table AS bindings
    WHERE bindings.service_instance_type = 'cloud'
);

$first_cloud_ba = (
    SELECT
        service_instance_id, 
        billing_account_id
    FROM $cloud_ba
    WHERE bind_order = 1
);

$cloud_created_dates = (
    SELECT
        clouds.cloud_id                             AS cloud_id,
        bindings.billing_account_id                 AS billing_account_id,
        DateTime::MakeTimestamp(clouds.created_at)  AS event_timestamp
    FROM $clouds_table AS clouds
        LEFT JOIN $first_cloud_ba AS bindings
        ON bindings.service_instance_id = clouds.cloud_id
);


INSERT INTO $destination_path WITH TRUNCATE
SELECT
  "cloud_created"                                                   AS event_type,
  CAST(DateTime::MakeDatetime(c.event_timestamp) AS Int64?)         AS event_timestamp,
  c.cloud_id                                                        AS event_entity_id,
  c.billing_account_id                                              AS billing_account_id,
  $get_md5(AsTuple("cloud_created", event_timestamp, cloud_id))     AS event_id,
  "iam"                                                             AS event_group,
  "cloud"                                                           AS event_entity_type,
  $format_msk_date_by_timestamp(event_timestamp)                    AS msk_event_dt,
  $format_msk_datetime_by_timestamp(event_timestamp)                AS msk_event_dttm,
  $format_msk_hour_by_timestamp(event_timestamp)                    AS msk_event_hour,
  $format_msk_month_name_by_timestamp(event_timestamp)              AS msk_event_month_name,
  $format_msk_quarter_by_timestamp(event_timestamp)                 AS msk_event_quarter,
  $format_msk_half_year_by_timestamp(event_timestamp)               AS msk_event_half_year,
  $format_msk_year_by_timestamp(event_timestamp)                    AS msk_event_year,
FROM $cloud_created_dates AS c
ORDER BY event_type, event_timestamp, event_entity_id;
