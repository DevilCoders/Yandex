PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $concat_path, $get_missing_or_updated_tables;
IMPORT `datetime` SYMBOLS  $get_date_range_inclusive, $parse_iso8601_string, $from_utc_ts_to_msk_dt;

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

$date_parse = DateTime::Parse("%Y-%m-%d");

$cluster = {{ cluster -> table_quote() }};
$start_of_history = {{ param["start_of_history"] -> quote() }};
$updates_start_dt = CAST(CurrentUtcDate() - DateTime::IntervalFromDays(3) as DateTime);
$load_iteration_limit = 50;

$tables_to_load = (
    SELECT *
    FROM $get_missing_or_updated_tables($cluster, $src_folder, $dst_folder, $start_of_history, $updates_start_dt, $load_iteration_limit)
);

DEFINE ACTION $insert_partition($date) AS
    $source_table = $concat_path($src_folder, $date);
    $destination_table = $concat_path($dst_folder, $date);

   INSERT INTO $destination_table WITH TRUNCATE
    SELECT
        DateTime::MakeDate($date_parse(created))                            AS created,
        licensed_services                                                   AS licensed_services,
        org_id                                                              AS org_id,
        uid                                                                 AS uid,
        $from_utc_ts_to_msk_dt($parse_iso8601_string(for_date))             AS for_date_dttm_local,
        $parse_iso8601_string(for_date)                                     AS for_date_ts
    FROM $source_table
    ORDER BY uid
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
