use hahn;
$format = DateTime::Format("%Y-%m-%d %H:%M:00");
$to_datetime = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::ParseIso8601($str)) };
$cloud_folder_dict = "//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict";

$format2 = DateTime::Format("%Y-%m-%dT%H:%M:%S");
$start_half_hour = $format2(DateTime::MakeDatetime(DateTime::StartOfDay(CurrentUtcDatetime())) );
$end_half_hour = $format2(DateTime::MakeTzDatetime(DateTime::StartOf(CurrentTzDatetime( "Europe/Moscow"), Interval("PT30M"))));

 
/* собираем соответсвие созданные folder_id и billing_account_id */
DEFINE SUBQUERY
$cloud_ba_help() AS 
    SELECT 
        distinct
        cloud_id, 
        billing_account_id,
        ts
    FROM $cloud_folder_dict
    WHERE 
        cloud_id IS not NULL
        AND billing_account_id IS not NULL
    UNION ALL
    SELECT 
        distinct
        cloud_id, 
        billing_account_id,
        $format(CurrentUtcTimestamp()) AS ts
    FROM $cloud_folder_dict
    WHERE 
        cloud_id IS not NULL
        AND billing_account_id IS not NULL
                
END DEFINE;

DEFINE SUBQUERY              
$cloud_ba_table() AS (
    SELECT 
        cloud_ba_1.cloud_id AS cloud_id,
        cloud_ba_1.billing_account_id AS billing_account_id,
        folder_cloud.folder_id AS folder_id,
        cloud_ba_1.ts AS ts_from,
        min(cloud_ba_2.ts) AS ts_till
    FROM $cloud_ba_help() AS cloud_ba_1
    INNER JOIN (SELECT DISTINCT 
                    folder_id,
                    cloud_id
                FROM `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict`) AS folder_cloud ON 
                folder_cloud.cloud_id=cloud_ba_1.cloud_id
    LEFT JOIN $cloud_ba_help() AS cloud_ba_2 ON cloud_ba_2.cloud_id = cloud_ba_1.cloud_id
    WHERE 
        cloud_ba_2.ts != cloud_ba_1.ts
        AND cloud_ba_1.ts < cloud_ba_2.ts
    GROUP BY 
        cloud_ba_1.cloud_id,
        cloud_ba_1.billing_account_id,
        folder_cloud.folder_id,
        cloud_ba_1.ts
    );
END DEFINE;

DEFINE SUBQUERY 
$new_project_create() AS (
    SELECT 
        df_log.folder_id AS folder_id,
        cloud_ba_table.billing_account_id AS billing_account_id,
        $to_datetime(df_log.iso_eventtime) AS new_project_create_time
    FROM (SELECT 
                df_log.folder_id AS folder_id,
                df_log._rest AS _rest,
                df_log.iso_eventtime AS iso_eventtime
          FROM RANGE (`//home/logfeller/logs/yc-ai-prod-logs-users-ml-platform/1d`,
                    '2020-12-01', CAST(CurrentUtcDate() AS String)) AS df_log
        WHERE  
            type = 'project'
        UNION ALL
        (SELECT 
                df_log.folder_id AS folder_id,
                df_log._rest AS _rest,
                df_log.iso_eventtime AS iso_eventtime
          FROM RANGE (`//home/logfeller/logs/yc-ai-prod-logs-users-ml-platform/30min`,
                    $start_half_hour, $end_half_hour) AS df_log
        WHERE  
            type = 'project')
            ) AS df_log
    INNER JOIN $cloud_ba_table()  AS cloud_ba_table ON cloud_ba_table.folder_id=df_log.folder_id
    WHERE
        Yson::LookupString(Yson::FromStringDict(_rest), "action")="create"
       AND cloud_ba_table.ts_from<df_log.iso_eventtime
        AND cloud_ba_table.ts_till>df_log.iso_eventtime
        );
END DEFINE;

EXPORT $new_project_create;