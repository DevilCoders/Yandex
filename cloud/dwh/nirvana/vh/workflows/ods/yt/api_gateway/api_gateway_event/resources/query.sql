PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $concat_path, $get_missing_or_updated_tables;
IMPORT `datetime` SYMBOLS $get_date_range_inclusive;

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

   INSERT INTO $destination_table WITH TRUNCATE
    SELECT
        request_id,
        grpc_service,
        grpc_method,
        user_agent,
        iso_eventtime
    FROM $source_table
    WHERE app = 'gateway'
        AND status = 200
        AND grpc_status_code = 0
        AND authority NOT LIKE '%yandex-team.ru%'
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
