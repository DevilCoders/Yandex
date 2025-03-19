USE hahn;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

$replace = Re2::Replace('([^T]*)T(.*)');
$now = CurrentUtcTimestamp();
$format = DateTime::Format("%Y-%m-%d");
$prev_day_start = $format(DateTime::StartOfDay(CAST($now AS Date) - Interval("P1D")));
$table_name = "//logs/yc-antifraud-overlay-flows-stats/1d/" || $prev_day_start;
$ba_cloud_folder = '//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict';
$res_table = '//home/cloud_analytics/import/network-logs/db-on-vm/data';

$new_logs = (
    SELECT 
        replace_date as `date`,
        cloud_id,
        db
    FROM (
        SELECT DISTINCT
            $replace(setup_time, "\\1") AS replace_date,
            cloud_id,
            CASE 
                WHEN sport IN (9000,8123,8443) THEN "clickhouse"
                WHEN sport IN (6432, 5432) THEN 'postgresql'
                WHEN sport IN (3306) THEN 'mysql'
                WHEN sport IN (6371) THEN 'redis'
                ELSE "other"
            END AS db
        FROM $table_name
        WHERE sport IN (6432, 5432, 3306, 6371, 9000, 8123, 8443)
    )
    WHERE replace_date >= $prev_day_start
);


$add_last_day = (
    SELECT 
        DISTINCT
        `date`,
        `last_billing_account_id` AS `billing_account_id`,
        `db`
    FROM $new_logs as logs
    LEFT JOIN (
        SELECT DISTINCT 
            `cloud_id`,
            FIRST_VALUE(`billing_account_id`) IGNORE NULLS OVER w AS `last_billing_account_id`
        FROM $ba_cloud_folder
        WINDOW w AS (PARTITION BY cloud_id ORDER BY ts DESC)
    ) AS billing_info
    ON logs.`cloud_id` = billing_info.`cloud_id`
);

INSERT INTO $res_table WITH TRUNCATE 
    SELECT 
        `date`,
        `billing_account_id`,
        `db`
    FROM $add_last_day
    UNION ALL
    SELECT 
        `date`,
        `billing_account_id`,
        `db`
    FROM $res_table
    WHERE `date` < $prev_day_start
;