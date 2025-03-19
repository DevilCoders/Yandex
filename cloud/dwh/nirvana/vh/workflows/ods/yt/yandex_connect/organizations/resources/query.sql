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
        admins                                                              AS admins,
        deputy_admins                                                       AS deputy_admins,
        CAST(first_debt_act_date AS String)                                 AS first_debt_act_date,
        id                                                                  AS organization_id,
        CAST(language AS String)                                            AS language,
        CAST(name AS String)                                                AS name,
        Yson::ConvertToString(organization_type)                            AS organization_type,
        DateTime::MakeDate($date_parse(registration_date))                  AS registration_date,
        services                                                            AS services,
        CAST(source AS String)                                              AS source,
        CAST(subscription_plan AS String)                                   AS subscription_plan,
        CAST(tld AS String)                                                 AS tld,
        $from_utc_ts_to_msk_dt($parse_iso8601_string(for_date))             AS for_date_dttm_local,
        $parse_iso8601_string(for_date)                                     AS for_date_ts
    FROM $source_table
    ORDER BY organization_id
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
