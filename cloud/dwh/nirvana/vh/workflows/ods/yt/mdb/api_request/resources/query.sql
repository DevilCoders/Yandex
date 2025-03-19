PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $concat_path, $get_missing_or_updated_tables;
IMPORT `datetime` SYMBOLS $get_date_range_inclusive;

$src_python_api_folder = {{ param["source_python_api_folder_path"] -> quote() }};
$src_go_api_folder = {{ param["source_go_api_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

$cluster = {{ cluster -> table_quote() }};
$start_of_history = {{ param["start_of_history"] -> quote() }};
$updates_start_dt = CAST(CurrentUtcDate() - DateTime::IntervalFromDays(7) as DateTime);
$load_iteration_limit = 50;

$tables_to_load = (
    SELECT t1.column0
    FROM $get_missing_or_updated_tables($cluster, $src_python_api_folder, $dst_folder, $start_of_history, $updates_start_dt, $load_iteration_limit) AS t1
    JOIN $get_missing_or_updated_tables($cluster, $src_go_api_folder, $dst_folder, $start_of_history, $updates_start_dt, $load_iteration_limit) AS t2
    ON t1.column0 = t2.column0
);

DEFINE ACTION $insert_partition($date) AS
    $source_python_api_table = $concat_path($src_python_api_folder, $date);
    $source_go_api_table = $concat_path($src_go_api_folder, $date);
    $destination_table = $concat_path($dst_folder, $date);

    INSERT INTO $destination_table WITH TRUNCATE
    SELECT
        mdb_go_api.request_id                                                           AS request_id,
        mdb_go_api.user_id                                                              AS user_id,
        Yson::ConvertToString(DictLookup(mdb_go_api._rest, 'user_type'))                AS user_type,
        Yson::ConvertToString(DictLookup(mdb_go_api._rest, 'cloud.ext_id'))             AS cloud_id,
        Yson::ConvertToString(DictLookup(mdb_go_api._rest, 'folder.ext_id'))            AS folder_id,
        Yson::ConvertToString(DictLookup(mdb_go_api._rest, 'cluster.id'))               AS cluster_id,
        mdb_go_api.iso_eventtime                                                        AS iso_eventtime,
        mdb_go_api.endpoint                                                             AS endpoint
    FROM $source_go_api_table AS mdb_go_api

    UNION ALL

    SELECT
        mdb_python_api.request_id                                                       AS request_id,
        mdb_python_api.user_id                                                          AS user_id,
        Yson::ConvertToString(Yson::ParseJson(mdb_python_api.authentication).user_type) AS user_type,
        mdb_python_api.cloud_ext_id                                                     AS cloud_id,
        mdb_python_api.folder_ext_id                                                    AS folder_id,
        NULL                                                                            AS cluster_id,
        mdb_python_api.iso_eventtime                                                    AS iso_eventtime,
        mdb_python_api.endpoint                                                         AS endpoint
    FROM $source_python_api_table AS mdb_python_api
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
