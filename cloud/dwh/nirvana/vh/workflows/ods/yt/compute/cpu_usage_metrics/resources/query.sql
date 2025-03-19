PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime_ms, $from_utc_ts_to_msk_dt;
IMPORT `helpers` SYMBOLS $lookup_string;
IMPORT `tables` SYMBOLS $concat_path, $get_missing_or_updated_tables;

$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

$cluster = {{ cluster -> table_quote() }};
$start_of_history = {{ param["start_of_history"] -> quote() }};
$updates_start_dt = CAST(CurrentUtcDate() - DateTime::IntervalFromDays(7) as DateTime);
$load_iteration_limit = 50;

$tables_to_load = (
    SELECT *
    FROM $get_missing_or_updated_tables($cluster, $src_folder, $dst_folder, $start_of_history, $updates_start_dt, $load_iteration_limit)
);

DEFINE ACTION $insert_partition($date) AS
    $source_table = $concat_path($src_folder, $date);
    $destination_table = $concat_path($dst_folder, $date);

    $extracted_metrics = (
        SELECT
            $get_datetime_ms(CAST(ts AS Uint64)) AS ts,
            value                                AS value,
            $get_string(labels, 'cloud_id')      AS cloud_id,
            $get_string(labels, 'folder_id')     AS folder_id,
            $get_string(labels, 'name')          AS name,
            $get_string(labels, 'resource_id')   AS resource_id,
            $get_string(labels, 'resource_type') AS resource_type,
        FROM $source_table AS cpu_usage
    );
    
    INSERT
        INTO $destination_table WITH TRUNCATE
    SELECT
       ts                           AS metric_ts,
       $from_utc_ts_to_msk_dt(ts)   AS metric_dttm_local,
       value                        AS metric_value,
       cloud_id                     AS cloud_id,
       folder_id                    AS folder_id,
       name                         AS metric_name,
       resource_id                  AS resource_id,
       resource_type                AS resource_type,
    FROM $extracted_metrics AS cpu_usage
    ORDER BY metric_ts ASC

END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
