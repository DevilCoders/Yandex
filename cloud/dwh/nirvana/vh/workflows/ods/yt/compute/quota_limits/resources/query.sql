PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $parse_hour_date_format_to_ts_msk, $get_date_range_inclusive, $get_datetime, $format_date, $MSK_TIMEZONE;
IMPORT `tables` SYMBOLS $concat_path, $get_all_non_empty_tables_w_offset;

$cluster = {{ cluster -> table_quote() }};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

-- Reload last 2 days:
-- Can be usefull in case job do not work several hours
-- or stop working near days boundary (23:59 -> 00:00)
$window_start_ts_str = $format_date(AddTimezone(CurrentUtcTimestamp(), $MSK_TIMEZONE) - Interval('P1D')) || 'T00:00:00';

-- skip first non empty row since it is most likely used by Data transfer
$window_end_ts_str = (
    SELECT TableName(Path) FROM $get_all_non_empty_tables_w_offset($src_folder, 1, 1, $cluster)
);

$dates_list = (
    $get_date_range_inclusive(
        $parse_hour_date_format_to_ts_msk($window_start_ts_str),
        $parse_hour_date_format_to_ts_msk($window_end_ts_str))
);

$source_table = (
    SELECT
        r.*,
        $get_datetime($parse_hour_date_format_to_ts_msk(TableName())) AS `timestamp`,
        $format_date($parse_hour_date_format_to_ts_msk(TableName()))  AS `date`
    FROM RANGE($src_folder, $window_start_ts_str, $window_end_ts_str) AS r
);


DEFINE ACTION $insert_partition($date) AS
    $destination_table = $concat_path($dst_folder, $date);

   INSERT INTO $destination_table WITH TRUNCATE
    SELECT
        container_id                                                AS cloud_id,
        resource_name                                               AS quota_name,
        `limit`                                                     AS `limit`,
        `timestamp`                                                 AS `timestamp`
    FROM $source_table
    WHERE `date` = $date
    ORDER BY cloud_id
END DEFINE;

EVALUATE FOR $date IN $dates_list DO BEGIN
    DO $insert_partition($date)
END DO;
