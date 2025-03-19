PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $format_hour, $parse_hour_date_format_to_dt_msk, $MSK_TIMEZONE;

$cluster = {{ cluster -> table_quote() }};
$src_quota_limits_folder = {{ param["quota_limits_folder"] -> quote() }};
$src_quota_usage_folder = {{ param["quota_usage_folder"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

-- Create DM only for last 90 days
$window_start_date = $format_hour(AddTimezone(CurrentUtcDate(), $MSK_TIMEZONE)- Interval('P90D'));
$window_end_date = $format_hour(AddTimezone(CurrentUtcDate(), $MSK_TIMEZONE));

$quota_limits = ( SELECT * FROM RANGE($src_quota_limits_folder, $window_start_date, $window_end_date) );
$quota_usage = ( SELECT * FROM RANGE($src_quota_usage_folder, $window_start_date, $window_end_date) );

$usage_with_gaps = (
    SELECT
                    NVL(quota_usage.cloud_id, quota_limits.cloud_id)       AS cloud_id,
                    NVL(quota_usage.`timestamp`, quota_limits.`timestamp`) AS `timestamp`,
                    NVL(quota_usage.quota_name, quota_limits.quota_name)   AS quota_name,
                    quota_limits.limit                                     AS limit,
                    quota_usage.usage                                      AS `usage`
                FROM $quota_usage as quota_usage
                FULL JOIN $quota_limits as quota_limits USING (cloud_id, quota_name ,`timestamp`)
);

$all_dates = (
    SELECT
     cloud_id,
     quota_name,
     Datetime::MakeDatetime(DateTime::FromSeconds(CAST (DateTime::ToSeconds(dt)+hours*3600 AS uint32))) AS `timestamp`
    FROM (
        SELECT cloud_id, quota_name, dt, ListFromRange(0,24) as hours
        FROM (
            SELECT cloud_id, quota_name, DateTime::MakeDate(`timestamp`) AS dt
            FROM $usage_with_gaps
        )
        GROUP BY cloud_id, quota_name, dt
    )
    FLATTEN BY hours
);


$result = (
SELECT
    cloud_id,
    `timestamp`,
    quota_name,
    NVL(`limit`, prev_quota_limit) AS quota_limit,
    NVL(`usage`, prev_usage) AS `usage`
FROM (
        SELECT
            usage_with_gaps.*,
            LAST_VALUE(`limit`) IGNORE NULLS OVER w      AS prev_quota_limit,
            LAST_VALUE(`usage`) IGNORE NULLS OVER w      AS prev_usage
            FROM (
                SELECT
                    t1.cloud_id AS cloud_id,
                    t1.quota_name AS quota_name,
                    t1.`timestamp` AS `timestamp`,
                    t2.`limit` AS `limit`,
                    t2.`usage` AS `usage`
                FROM $all_dates AS t1
                LEFT JOIN $usage_with_gaps AS t2
                    USING(cloud_id, quota_name, `timestamp`)
        ) AS usage_with_gaps
        WINDOW w AS (
            PARTITION BY cloud_id, quota_name
            ORDER BY `timestamp`
        )
    )
);

INSERT INTO $dst_table
WITH TRUNCATE
SELECT *
FROM $result
ORDER BY cloud_id, quota_name, `timestamp`
