PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $format_utc_datetime_by_timestamp;

$folders_table = {{param["folders_table"]->quote()}};
$service_instances_table = {{param["service_instances_table"]->quote()}};
$destination_path = {{input1->table_quote()}};

-- get clouds
$clouds = (
  SELECT DISTINCT
    service_instance_id AS cloud_id,
    billing_account_id,
    effective_time AS epoch,
    $format_utc_datetime_by_timestamp(effective_time) AS datetime_str
  FROM $service_instances_table
  WHERE service_instance_type = "cloud"
);

-- get folders
$folders = (
  SELECT DISTINCT
    folder_id,
    cloud_id
  FROM $folders_table
);

-- create result table
$result = (
  SELECT
    folders.folder_id as folder_id,
    folders.cloud_id as cloud_id,
    clouds.billing_account_id as billing_account_id,
    clouds.epoch as epoch,
    clouds.datetime_str AS ts
  FROM $folders AS folders
  LEFT JOIN $clouds AS clouds USING(cloud_id)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  *
FROM $result
ORDER BY folder_id, cloud_id, billing_account_id;
