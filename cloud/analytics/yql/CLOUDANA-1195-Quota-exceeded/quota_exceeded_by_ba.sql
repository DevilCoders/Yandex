use hahn;

DEFINE SUBQUERY $quota_exceeded_consumption() AS

$quota_exceeded_folder = "//home/cloud_analytics/dwh/ods/compute/quota_exceeded_log";

$format = DateTime::Format("%Y-%m-%d %H:%M:00");
$date_time = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::ParseIso8601($str)) };
$start_of_month = ($str) ->{RETURN DateTime::MakeDatetime(DateTime::StartOfMonth(DateTime::ParseIso8601($str)))};
$start_of_month_3_befor = ($str) ->{RETURN DateTime::MakeDatetime(DateTime::ShiftMonths(DateTime::StartOfMonth(DateTime::ParseIso8601($str)), -3))};

-- события quota exceeded
    $quota_exceeded = (
        SELECT cloud_id,
            resource_name AS quota_type,
            event_dttm,
            request_id AS event_id
        FROM RANGE($quota_exceeded_folder, "2020-10-09", CAST(CurrentUtcDate() AS String))
        WHERE cloud_id != ''
    );

/*вспомогательные таблицы для связки billing_account_id и cloud_id*/
    $cloud_ba_help = (
        SELECT 
            distinct
            cloud_id, 
            billing_account_id,
            ts
        FROM `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict`
        WHERE 
            cloud_id IS not NULL
            AND billing_account_id IS not NULL
        UNION ALL
        (SELECT 
            distinct
            cloud_id, 
            billing_account_id,
            $format(CurrentUtcTimestamp()) AS ts
        FROM `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict`
        WHERE 
            cloud_id IS not NULL
            AND billing_account_id IS not NULL
        )
    );

    $cloud_ba_table = (
        SELECT 
            cloud_ba_1.cloud_id AS cloud_id,
            cloud_ba_1.billing_account_id AS billing_account_id,
            cloud_ba_1.ts AS ts_from,
            min(cloud_ba_2.ts) AS ts_till
       FROM $cloud_ba_help AS cloud_ba_1
       LEFT JOIN $cloud_ba_help AS cloud_ba_2 ON cloud_ba_2.cloud_id = cloud_ba_1.cloud_id
       WHERE 
            cloud_ba_2.ts != cloud_ba_1.ts
            AND cloud_ba_1.ts < cloud_ba_2.ts
       GROUP BY 
            cloud_ba_1.cloud_id,
            cloud_ba_1.billing_account_id,
            cloud_ba_1.ts
    );

    /*добавляем к событиям quota_exceeded соответствие cloud_id с  billing_account_id*/
    $quota_exceeded_with_ba = (
        SELECT
            quota_exceeded.cloud_id AS cloud_id,
            cloud_ba_table.billing_account_id AS billing_account_id,
            quota_exceeded.quota_type AS quota_type,
            quota_exceeded.event_dttm AS quota_exceeded_dttm,
            quota_exceeded.event_id AS event_id
        FROM $quota_exceeded AS quota_exceeded
        INNER JOIN $cloud_ba_table AS cloud_ba_table ON cloud_ba_table.cloud_id = quota_exceeded.cloud_id
        WHERE 
            quota_exceeded.event_dttm >= cloud_ba_table.ts_from
            AND quota_exceeded.event_dttm < cloud_ba_table.ts_till
    );
    
    $quota_exceeded_with_ba = (
        SELECT
            quota_exceeded.cloud_id AS cloud_id,
            cloud_ba_table.billing_account_id AS billing_account_id,
            quota_exceeded.quota_type AS quota_type,
            quota_exceeded.event_dttm AS quota_exceeded_dttm,
            quota_exceeded.event_id AS event_id
        FROM $quota_exceeded AS quota_exceeded
        INNER JOIN $cloud_ba_table AS cloud_ba_table ON cloud_ba_table.cloud_id = quota_exceeded.cloud_id
        WHERE 
            quota_exceeded.event_dttm >= cloud_ba_table.ts_from
            AND quota_exceeded.event_dttm < cloud_ba_table.ts_till
    );

    /* Добавляем информацию о segment ba_state и ba_usage_status*/
    $quota_ba_features = (
        SELECT DISTINCT
                quota_exceeded_with_ba.billing_account_id AS billing_account_id,
                quota_exceeded_with_ba.cloud_id AS cloud_id,
                analitic_cube.ba_usage_status AS ba_usage_status,
                analitic_cube.segment AS segment,
                analitic_cube.ba_state AS ba_state,
                quota_exceeded_with_ba.event_id AS event_id,
                quota_exceeded_with_ba.quota_type AS quota_type,
                quota_exceeded_with_ba.quota_exceeded_dttm AS quota_exceeded_dttm
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube` AS analitic_cube
        INNER JOIN $quota_exceeded_with_ba AS quota_exceeded_with_ba
                ON quota_exceeded_with_ba.billing_account_id=analitic_cube.billing_account_id
        WHERE 
            analitic_cube.event='day_use'
            AND SUBSTRING(analitic_cube.event_time, 0, 10) = SUBSTRING(quota_exceeded_with_ba.quota_exceeded_dttm, 0, 10)
    );
    
    SELECT
        quota_exceeded.billing_account_id AS billing_account_id,
        quota_exceeded.cloud_id  AS cloud_id,
        quota_exceeded.ba_usage_status AS ba_usage_status,
        quota_exceeded.segment AS segment,
        quota_exceeded.ba_state AS ba_state,
        quota_exceeded.quota_type AS quota_type,
        $date_time(quota_exceeded.quota_exceeded_dttm) AS event_dttm,
        quota_exceeded.event_id AS event_id,
        SUM(`cube`.real_consumption)AS real_consumption,
        SUM(`cube`.real_consumption_vat) AS real_consumption_vat
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube` AS `cube`
    INNER JOIN $quota_ba_features AS quota_exceeded 
        ON quota_exceeded.billing_account_id = `cube`.billing_account_id
    WHERE 
        `cube`.event='day_use'
        AND `cube`.ba_usage_status != 'service' 
        AND $date_time(`cube`.event_time) < $start_of_month(quota_exceeded.quota_exceeded_dttm)
        AND $date_time(`cube`.event_time) > $start_of_month_3_befor(quota_exceeded.quota_exceeded_dttm)
    GROUP BY 
        quota_exceeded.billing_account_id,
        quota_exceeded.cloud_id,
        quota_exceeded.quota_type,
        quota_exceeded.quota_exceeded_dttm,
        quota_exceeded.event_id,
        quota_exceeded.ba_usage_status,
        quota_exceeded.segment,
        quota_exceeded.ba_state
    
END DEFINE;

EXPORT $quota_exceeded_consumption;