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

    INSERT
        INTO $destination_table WITH TRUNCATE
    SELECT
        request_id,
        service_name,
        service_method,
        ua AS user_agent,
        iso_eventtime,
        url
    FROM (
        SELECT
            String::SplitToList(url, '/')[1] AS url1,
            url,
            String::SplitToList(String::ReplaceAll(url, 'root/', ''), '/')[2] AS service_name,
            String::SplitToList(String::ReplaceAll(url, 'root/', ''), '/')[3] AS service_method,
            CASE WHEN Yson::Contains(DictLookup(_rest, '@fields').req, 'user_agent')
                THEN Yson::ConvertToString(DictLookup(_rest, '@fields').req.user_agent)
                ELSE NULL END AS ua,
            req_id AS request_id,
            iso_eventtime
        FROM $source_table
        WHERE method IN ('GET','POST')
            AND status = 200
            AND url NOT IN ('/ping')
    )
    WHERE url1 IN ('api', 'gateway', 'grpc')
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
