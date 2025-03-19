PRAGMA yt.InferSchema = '1';

PRAGMA Library("tables.sql");

IMPORT `tables` SYMBOLS $get_all_daily_tables;

$cluster = {{cluster->table_quote()}};

$raw_folder_daily   = {{ param["raw_folder_daily"] -> quote() }};
$raw_folder_monthly = {{ param["raw_folder_monthly"] -> quote() }};
$destination_path   = {{input1->table_quote()}};

$replace_cluster = Re2::Replace("^cluster-");

$table_paths = ( SELECT * FROM $get_all_daily_tables($raw_folder_daily, $cluster) );

$daily_result = (
  SELECT
    $replace_cluster(CAST(host as String), '') as cluster,
    CAST(sensor as String) as sensor,
    CAST(type as String) as media,
    CAST(value as Double) as value,
    CAST(CAST(`timestamp` as Uint64) / 1000 as Datetime) as `created_timestamp`,
  FROM EACH($table_paths)
  WHERE value != "NaN"
);

$monthly_result = (
  SELECT
    $replace_cluster(CAST(host as String), '') as cluster,
    CAST(sensor as String) as sensor,
    CAST(type as String) as media,
    CAST(value as Double) as value,
    CAST(CAST(`timestamp` as Uint64) / 1000 as Datetime) as `created_timestamp`,
  FROM RANGE($raw_folder_monthly)
  WHERE value != "NaN"
);

$result = (
  SELECT *
  FROM (
      SELECT * FROM $daily_result
      UNION ALL
      SELECT * FROM $monthly_result
  )
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  *
FROM $result
ORDER BY created_timestamp, cluster;
