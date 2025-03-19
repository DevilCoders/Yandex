$concat_path = ($folder, $path) -> (
    IF(String::EndsWith($folder, '/'), $folder, $folder || '/')
    || IF(String::StartsWith($path, '/'), String::RemoveFirst($path, '/'), $path)
);

-- get maximum table path in directory (logfeller-format only)
DEFINE SUBQUERY $get_max_daily_table_path($folder, $cluster) AS
    USE yt:$cluster;

    SELECT
        MAX(Path) as path
    FROM FOLDER($concat_path($folder, "1d"), "row_count")
    WHERE Type = "table" AND Yson::LookupInt64(Attributes, "row_count") > 0
END DEFINE;

-- get last not empty table path
DEFINE SUBQUERY $select_last_not_empty_table($folder, $cluster) AS
    USE yt:$cluster;

    $path = (
        SELECT
            MAX(Path)
        FROM FOLDER($folder, "row_count")
        WHERE Type = "table" AND Yson::LookupInt64(Attributes, "row_count") > 0
    );
    SELECT * FROM CONCAT($path)
END DEFINE;

-- extracts `Date` from maximum table path in directory
DEFINE SUBQUERY $get_max_date_from_table_path($folder, $cluster) AS
    USE yt:$cluster;

    SELECT
        MAX(CAST(SUBSTRING(Path, RFIND(Path, '/') + 1u) as Date)) as `max_date`
    FROM FOLDER($folder, "row_count")
    WHERE Type="table" AND Yson::LookupInt64(Attributes, "row_count") > 0
END DEFINE;


-- Get all tables from daily folder ($folder/1d/*) with current day hours folder ($folder/1h/*) (like LogFeller folders format).
-- Returns list non empty tables.
DEFINE SUBQUERY $get_all_daily_tables($folder, $cluster) AS
    USE yt:$cluster;

    $get_next_date = ($dt) -> (CAST((CAST($dt as Date) + Interval('P1D')) as String));

    $max_day_table_path = (
        SELECT
            *
        FROM $get_max_daily_table_path($folder, $cluster)
    );

    SELECT
        AGGREGATE_LIST(Path) AS paths
    FROM (
        SELECT
            Path
        FROM FOLDER($concat_path($folder, "1d"), "row_count")
        WHERE Type = "table"
            AND Yson::LookupInt64(Attributes, "row_count") > 0
            AND ($max_day_table_path IS NULL OR Path <= $max_day_table_path)

        UNION ALL

        SELECT
            Path
        FROM FOLDER($concat_path($folder, "1h"), "row_count")
        WHERE Type = "table"
            AND Yson::LookupInt64(Attributes, "row_count") > 0
            AND ($max_day_table_path IS NULL OR TableName(Path) >= $get_next_date(TableName($max_day_table_path)))
    )
END DEFINE;

-- Get all tables from daily folder ($folder/1d/*) with current day hours folder ($folder/1h/*) (like LogFeller folders format)  with month filter.
-- Returns list non empty tables with month filter.
DEFINE SUBQUERY $get_all_daily_tables_from_month($folder, $cluster, $month) AS
    USE yt:$cluster;

    $format_date = DateTime::Format("%Y-%m-%d");
    $get_next_date = ($dt) -> (CAST((CAST($dt as Date) + Interval('P1D')) as String));
    $get_period_start_date = $format_date(CAST($month as Date) - Interval('P1D'));
    $get_period_end_date   = $format_date( DateTime::ShiftMonths(CAST($month as Date) + Interval('P5D'), 1) );

    $max_day_table_path = (
        SELECT
            *
        FROM $get_max_daily_table_path($folder, $cluster)
    );

    SELECT
        AGGREGATE_LIST(Path) AS paths
    FROM (
        SELECT
            Path
        FROM FOLDER($concat_path($folder, "1d"), "row_count")
        WHERE Type = "table"
            AND Yson::LookupInt64(Attributes, "row_count") > 0
            AND ($max_day_table_path IS NULL OR Path <= $max_day_table_path)
            AND TableName(Path) between $get_period_start_date and $get_period_end_date

        UNION ALL

        SELECT
            Path
        FROM FOLDER($concat_path($folder, "1h"), "row_count")
        WHERE Type = "table"
            AND Yson::LookupInt64(Attributes, "row_count") > 0
            AND ($max_day_table_path IS NULL OR TableName(Path) >= $get_next_date(TableName($max_day_table_path)))
            AND TableName(Path) between $get_period_start_date and $get_period_end_date
    )
END DEFINE;

-- check if table exists in folder
DEFINE SUBQUERY $is_table_exists($folder, $table_name, $cluster) AS
    USE yt:$cluster;

    SELECT
        count(*) > 0 as is_exists
    FROM FOLDER ($folder)
    WHERE Type = "table" AND TableName(Path) == $table_name
END DEFINE;

-- aggregate last (actual) snapshot from hourly logfeller log
DEFINE SUBQUERY $aggregate_logfeller_export_table($folder, $primary_key, $cluster) AS
    USE yt:$cluster;

    $all_rows = (
        SELECT *
        WITHOUT
            `_logfeller_timestamp`,
            `_rest`,
            `_stbx`,
            `_logfeller_index_bucket`,
            `export_hour`,
            `export_host`,
            `iso_eventtime`,
            `source_uri`
        FROM RANGE($folder)
    );

    $unique = (
        SELECT
            MAX_BY(TableRow(), `export_ts`)
        FROM $all_rows
        GROUP BY ChooseMembers(TableRow(), $primary_key)
    );

    SELECT * FROM $unique FLATTEN COLUMNS;
END DEFINE;

-- Selects last (actual) snapshot from transfer manager tables
DEFINE SUBQUERY $select_transfer_manager_table($tables_path, $cluster) AS
    USE yt:$cluster;
    $latest_fully_exported_table = (
        SELECT
            `Path`
        FROM (
            SELECT
                Yson::YPathUint64(Attributes, '/resource_usage/disk_space') as `DiskSpace`,
                `Type`,
                `Path`
            FROM FOLDER($tables_path, 'resource_usage')
        )
        WHERE
            `Type`='table' AND cast(`DiskSpace` as uint64) > 0
        ORDER BY
            `Path` DESC
        LIMIT 1 OFFSET 1
    );
    SELECT * FROM EACH(AsList($latest_fully_exported_table??""))
END DEFINE;

/**
    Selects last (actual) snapshot from datatransfer tables in case of snapshot ratotion approach.
    - $tables_path - folder where snapshot tables are stored.
    - $cluster - YT cluster.
    - $offset - how many latest tables need to skip. Use for skipping still loading tables depending on DT snapshoting schema.
**/
DEFINE SUBQUERY $select_datatransfer_snapshot_table($tables_path, $cluster, $offset) AS
    USE yt:$cluster;
    $latest_fully_exported_table = (
        SELECT `Path`
        FROM (
            SELECT
                Yson::YPathUint64(Attributes, '/resource_usage/disk_space') as `DiskSpace`,
                `Type`,
                `Path`
            FROM FOLDER($tables_path, 'resource_usage')
        )
        WHERE `Type`='table' AND cast(`DiskSpace` as uint64) > 0
        ORDER BY `Path` DESC
        LIMIT 1
        OFFSET $offset
    );
    SELECT * FROM EACH(AsList($latest_fully_exported_table??""))
END DEFINE;

/**
    Returns non empty tables.
 */
DEFINE SUBQUERY $get_all_non_empty_tables($folder_path, $cluster) AS
    USE yt:$cluster;

    SELECT
        `Path`
    FROM (
        SELECT
            Yson::YPathUint64(Attributes, '/resource_usage/disk_space') as `DiskSpace`,
            `Type`,
            `Path`
        FROM FOLDER($folder_path, 'resource_usage')
    )
    WHERE `Type`='table' AND cast(`DiskSpace` as uint64) > 0
    ORDER BY `Path` DESC
END DEFINE;

DEFINE SUBQUERY $get_all_non_empty_tables_w_offset($folder_path,$limit, $offset, $cluster) AS
      USE yt:$cluster;

      SELECT
        `Path`
      FROM (
          SELECT
              Yson::YPathUint64(Attributes, '/resource_usage/disk_space') as `DiskSpace`,
              `Type`,
              `Path`
          FROM FOLDER($folder_path, 'resource_usage')
      )
      WHERE `Type`='table' AND cast(`DiskSpace` as uint64) > 0
      ORDER BY `Path` DESC
      LIMIT $limit OFFSET $offset
END DEFINE;

/**
    Returns tables with modification time after $modification_time.
 */
DEFINE SUBQUERY $get_updated_tables($folder_path, $modification_time, $cluster) AS
    USE yt:$cluster;

    $ts_parse = DateTime::Parse("%Y-%m-%dT%H:%M:%SZ");

    SELECT
        `Path`
    FROM (
        SELECT
            `Path`                                                          AS `Path`,
            `Type`                                                          AS `Type`,
            Yson::YPathUint64(Attributes, '/resource_usage/disk_space')     AS `DiskSpace`,
            $ts_parse(Yson::YPathString(Attributes, '/modification_time'))  AS modified_at_dt
        FROM FOLDER($folder_path, 'modification_time;resource_usage')
        )
    WHERE
        `Type`='table'
        AND cast(`DiskSpace` as uint64) > 0
        AND modified_at_dt >= $modification_time;
END DEFINE;

DEFINE SUBQUERY $get_table_names($folder_path, $cluster) AS
    USE yt:$cluster;
    SELECT TableName(Path) as name
    FROM $get_all_non_empty_tables($folder_path, $cluster);
END DEFINE;


/**
    Returns tables with modification time before $modification_time.
 */
DEFINE SUBQUERY $get_old_tables($folder_path, $modification_time, $cluster) AS
    USE yt:$cluster;

    $ts_parse = DateTime::Parse("%Y-%m-%dT%H:%M:%SZ");

    SELECT
        `Path`
    FROM (
        SELECT
            `Path`                                                          AS `Path`,
            `Type`                                                          AS `Type`,
            Yson::YPathUint64(Attributes, '/resource_usage/disk_space')     AS `DiskSpace`,
            $ts_parse(Yson::YPathString(Attributes, '/modification_time'))  AS modified_at_dt
        FROM FOLDER($folder_path, 'modification_time;resource_usage')
        )
    WHERE
        `Type`='table'
        AND cast(`DiskSpace` as uint64) > 0
        AND modified_at_dt <= $modification_time;
END DEFINE;

/**
Returns table names that are in source path, but not int $destination_path

- $source_path path to tables folder
- $source_offset offset with table to start first, added only to support raw/ods layers.
in RAW layer (data transfer updates table with latest date).
 */
DEFINE SUBQUERY $get_missing_tables($source_path, $destination_path, $cluster) AS
    USE yt:$cluster;

    SELECT
        TableName(Path) AS table_name
    FROM $get_all_non_empty_tables($source_path, $cluster)
    WHERE TableName(Path) NOT IN (
        SELECT TableName(Path) AS table_name
        FROM $get_all_non_empty_tables($destination_path, $cluster)
    );
END DEFINE;

/**
Returns table names that are in source path, but not int $destination_path

- $source_path path to tables folder
- $source_offset offset with table to start first, added only to support raw/ods layers.
in RAW layer (data transfer updates table with latest date).
 */
DEFINE SUBQUERY $get_source_tables_that_not_in_target($source_path, $destination_path, $cluster) AS
    USE yt:$cluster;

    SELECT AGGREGATE_LIST(table_name) as tables
    FROM $get_missing_tables($source_path, $destination_path, $cluster);
END DEFINE;

/**
Returns table names that are in source path, but not in $destination_path.

- $cluster - name of YT cluster.
- $source_path - path to tables in source folder.
- $destination_path - path to tables in destination folder.
- $load_start_date - minimal date string (ex. '2021-01-31') to ignore older tables.
- $modification_time_to_pickup - datetime to track modifications of tables in $source_path.
- $limit - limit number of tables to be returned.
 */
DEFINE SUBQUERY $get_missing_or_updated_tables($cluster, $source_path, $destination_path, $load_start_date, $modification_time_to_pickup, $limit) AS
    USE yt:$cluster;


    $tables_to_load = (
        SELECT DISTINCT
            table_name
        FROM (
            SELECT table_name
            FROM $get_missing_tables($source_path, $destination_path, $cluster)

            UNION ALL

            SELECT TableName(Path) AS table_name
            FROM $get_updated_tables($source_path, $modification_time_to_pickup, $cluster)
        ) AS t
        WHERE table_name >= $load_start_date
        ORDER BY table_name DESC
        LIMIT $limit
    );

    SELECT AGGREGATE_LIST(table_name)
    FROM $tables_to_load

END DEFINE;


/**
Returns table names that are in source path, but not in $destination_path.

- $cluster - name of YT cluster.
- $source_path - list paths to tables in source folder.
- $destination_path - path to tables in destination folder.
- $load_start_date - minimal date string (ex. '2021-01-31') to ignore older tables.
- $days_qty_to_reload - datetime to track modifications of tables in $source_path.
- $reload_before_datetime - days qty to track modifications of tables in $destination_path.
- $limit - limit number of tables to be returned.
 */

DEFINE SUBQUERY $get_daily_tables_to_load($cluster, $source_path, $destination_path, $load_start_date, $days_qty_to_reload, $reload_before_datetime, $limit) AS
    USE yt:$cluster;

    $modification_time_to_pickup = CAST(CurrentUtcDate() - DateTime::IntervalFromDays($days_qty_to_reload) AS DateTime);


    $tables_to_load = (
            SELECT DISTINCT
                table_name
            FROM (
                -- пропущенные таблицы
                SELECT table_name
                FROM $get_missing_tables($source_path, $destination_path, $cluster)

                UNION ALL
                --обновленные таблицы для пересчета
                SELECT TableName(Path) AS table_name
                FROM $get_updated_tables($source_path, $modification_time_to_pickup, $cluster)

                UNION ALL
                -- старые таблицы для пересчета
                SELECT TableName(Path) AS table_name
                FROM $get_old_tables($destination_path, $reload_before_datetime, $cluster)
            ) AS t
            WHERE table_name >= $load_start_date
            ORDER BY table_name DESC
            LIMIT $limit
    );

    SELECT
        AGGREGATE_LIST(table_name) AS table
    FROM $tables_to_load

END DEFINE;


DEFINE SUBQUERY $get_table_information($cluster, $folder_path) AS
    USE yt:$cluster;

    $ts_parse = DateTime::Parse("%Y-%m-%dT%H:%M:%SZ");

    SELECT
        table_path,
        disk_space,
        created_dttm,
        modified_dttm
    FROM (
        SELECT
            `Path`                                                                          AS table_path,
            $ts_parse(Yson::YPathString(Attributes, '/creation_time'))                      AS created_dttm,
            $ts_parse(Yson::YPathString(Attributes, '/modification_time'))                  AS modified_dttm,
            cast(Yson::YPathUint64(Attributes, '/resource_usage/disk_space')  AS uint64)    AS disk_space
        FROM FOLDER($folder_path, 'modification_time;resource_usage;creation_time')
        WHERE `Type` = 'table'
        )
        WHERE disk_space > 0
END DEFINE;

DEFINE SUBQUERY $get_created_month_tables($cluster, $folder_path, $days_qty_to_reload) AS
    USE yt:$cluster;

    $created_dttm = CAST(CurrentUtcDate() - DateTime::IntervalFromDays($days_qty_to_reload) AS DateTime);

    SELECT DISTINCT
        table_name
    FROM (
        SELECT DISTINCT
            SUBSTRING(TableName(table_path), 0, 8) || '01' AS table_name
        FROM $get_table_information($cluster, $folder_path || '/1d')
        WHERE created_dttm >= $created_dttm
        UNION ALL
        SELECT DISTINCT
            SUBSTRING(TableName(table_path), 0, 8) || '01' AS table_name
        FROM $get_table_information($cluster, $folder_path || '/1h')
        WHERE created_dttm >= $created_dttm
          )
    ;
END DEFINE;

DEFINE SUBQUERY $get_old_month_tables($cluster, $folder_path, $modification_datetime) AS
    USE yt:$cluster;

    SELECT DISTINCT
        SUBSTRING(TableName(table_path),0,8)||'01'    AS table_name
    FROM $get_table_information($cluster, $folder_path)
    WHERE modified_dttm <= $modification_datetime
    ;
END DEFINE;

DEFINE SUBQUERY $get_miss_month_tables($cluster, $source_path, $destination_path) AS
    USE yt:$cluster;

    SELECT DISTINCT
        table_name
    FROM (
        SELECT DISTINCT
            SUBSTRING(TableName(table_path),0,8)||'01' AS table_name
        FROM $get_table_information($cluster, $source_path || '/1d')
        UNION ALL
        SELECT DISTINCT
            SUBSTRING(TableName(table_path),0,8)||'01' AS table_name
        FROM $get_table_information($cluster, $source_path || '/1h')
    )
    WHERE table_name NOT IN (
        SELECT SUBSTRING(TableName(table_path),0,8)||'01' AS table_name
        FROM $get_table_information($cluster, $destination_path)
    )
    ;
END DEFINE;


/**
Returns table names that are in source path, but not in $destination_path.

- $cluster - name of YT cluster.
- $source_path - list paths to tables in source folder.
- $destination_path - path to tables in destination folder.
- $load_start_date - minimal date string (ex. '2021-01-31') to ignore older tables.
- $days_qty_to_reload - datetime to track modifications of tables in $source_path.
- $reload_before_datetime - days qty to track modifications of tables in $destination_path.
- $limit - limit number of tables to be returned.
 */
DEFINE SUBQUERY $get_logfeller_monthly_tables_to_load($cluster, $source_path, $destination_path, $load_start_date, $days_qty_to_reload, $reload_before_datetime, $limit) AS
    USE yt:$cluster;

    $tables_to_load = (
        SELECT
            table_name
        FROM (
            SELECT DISTINCT
                table_name AS table_name
            FROM (
                --пропущенные таблицы
                SELECT table_name
                FROM $get_miss_month_tables($cluster, $source_path, $destination_path)

                UNION ALL
                -- обновленные таблицы для пересчета
                SELECT table_name
                FROM $get_created_month_tables($cluster, $source_path, $days_qty_to_reload)

                UNION ALL
                -- старые таблицы для пересчета
                SELECT table_name
                FROM $get_old_month_tables($cluster, $destination_path, $reload_before_datetime)
            ) AS t
            WHERE table_name >= $load_start_date
            ORDER BY table_name DESC
            LIMIT $limit
        ) AS t
    );

    SELECT
        AGGREGATE_LIST(table_name) as table
    FROM $tables_to_load

END DEFINE;


EXPORT $concat_path, $get_max_daily_table_path, $select_last_not_empty_table,
    $get_all_daily_tables, $get_all_daily_tables_from_month, $is_table_exists, $get_max_date_from_table_path,
    $aggregate_logfeller_export_table, $select_transfer_manager_table,
    $get_all_non_empty_tables, $get_all_non_empty_tables_w_offset, $get_source_tables_that_not_in_target,
    $get_missing_or_updated_tables, $get_daily_tables_to_load, $get_logfeller_monthly_tables_to_load,
    $select_datatransfer_snapshot_table;
