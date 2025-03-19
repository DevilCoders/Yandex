use hahn;

$result_table_path = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_daily_usage';
$result_table_without_ba_path = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_daily_usage_without_ba';

$ba_cloud_folder_dict_path = '//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict_new';
$ba_cloud_organization_dict_path =  '//home/cloud_analytics/dictionaries/ids/ba_cloud_organization_dict';
-- till 2022-04-13 inclusive
$datalens_requests_before_path = '//home/yandexbi/datalens-back/ext/production/requests';
-- after 2022-04-14
$datalens_requests_now_path = '//home/yandexbi/robot-datalens/usage-tracking/logs/datalens-back-ext-production-fast/1d';

-- format
$format_d = DateTime::Format("%Y-%m-%d");
$format_dt = DateTime::Format("%Y-%m-%d %H:%M:%S");
-- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
-- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
-- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$format_msk_date_by_timestamp = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_d));
$format_msk_datetime_by_timestamp = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_dt));
-- string to datetime
$parse_datetime = DateTime::Parse("%Y-%m-%d %H:%M:%S");
$str_to_datetime = ($str) -> (DateTime::MakeDatetime($parse_datetime($str)));


DEFINE ACTION $datalens_daily_usage_script() AS

    $user_info_raw = (
        SELECT
            $format_msk_date_by_timestamp($str_to_datetime(event_time)) as event_date, -- string -> msk_date
            $format_msk_datetime_by_timestamp($str_to_datetime(event_time)) as event_datetime, -- string -> msk_datetime
            cache_full_hit,
            user_id,
            CASE WHEN folder_id not like 'org_%' then folder_id else NULL end as folder_id,
            CASE WHEN folder_id like 'org_%' then SUBSTRING(folder_id, 4) else NULL end as organization_id,
            dataset_mode,
            CAST(execution_time as DOUBLE) as execution_time,
            connection_id,
            request_id,
            error,
            status,
            connection_type,
            cache_used,
            is_public,
            dataset_id,
            source,
            host
        FROM RANGE($datalens_requests_before_path,
                    `2020-04-15T00:00:00`,  
                    `2022-04-13T00:00:00`) 

        UNION ALL

        SELECT
            $format_msk_date_by_timestamp(event_time) as event_date, -- int -> utc -> string
            $format_msk_datetime_by_timestamp(event_time) as event_datetime, -- string -> msk_datetime
            cache_full_hit,
            user_id,
            CASE WHEN folder_id not like 'org_%' then folder_id else NULL end as folder_id,
            CASE WHEN folder_id like 'org_%' then SUBSTRING(folder_id, 4) else NULL end as organization_id,
            dataset_mode,
            CAST(execution_time as DOUBLE) as execution_time,
            connection_id,
            request_id,
            error,
            status,
            connection_type,
            cache_used,
            is_public,
            dataset_id,
            source,
            host
        FROM RANGE($datalens_requests_now_path)
    ); 

--  group by day
    $user_info_by_day = (
        SELECT
            datalens_usage.event_date                 as event_date,
            datalens_usage.cache_full_hit             as cache_full_hit,
            datalens_usage.folder_id                  as folder_id,
            datalens_usage.organization_id            as organization_id,
            datalens_usage.dataset_mode               as dataset_mode,
            datalens_usage.connection_id              as connection_id,
            datalens_usage.user_id                    as user_id,
            SUM(datalens_usage.execution_time)        as total_execution_time,
            AVG(datalens_usage.execution_time)        as avg_execution_time,
            MEDIAN(datalens_usage.execution_time)     as median_execution_time,
            COUNT(DISTINCT datalens_usage.request_id) as request_count,
            datalens_usage.status                     as status,
            datalens_usage.connection_type            as connection_type,
            datalens_usage.cache_used                 as cache_used,
            datalens_usage.is_public                  as is_public,
            datalens_usage.dataset_id                 as dataset_id,
            datalens_usage.source                     as source,
            datalens_usage.host                       as host
        FROM $user_info_raw as datalens_usage
        GROUP BY
            datalens_usage.event_date,
            datalens_usage.cache_full_hit,
            datalens_usage.folder_id,
            datalens_usage.organization_id,
            datalens_usage.dataset_mode,
            datalens_usage.connection_id,
            datalens_usage.user_id,
            datalens_usage.status,
            datalens_usage.connection_type,
            datalens_usage.cache_used,
            datalens_usage.is_public,
            datalens_usage.dataset_id,
            datalens_usage.source,
            datalens_usage.host
    );

    $join_user_ba = (
        SELECT
            datalens_usage.event_date                 as event_date,
            datalens_usage.cache_full_hit             as cache_full_hit,
            datalens_usage.folder_id                  as folder_id,
            datalens_usage.organization_id            as organization_id,
            datalens_usage.dataset_mode               as dataset_mode,
            datalens_usage.connection_id              as connection_id,
            datalens_usage.user_id                    as user_id,
            datalens_usage.total_execution_time       as total_execution_time,
            datalens_usage.avg_execution_time         as avg_execution_time,
            datalens_usage.median_execution_time      as median_execution_time,
            datalens_usage.request_count              as request_count,
            datalens_usage.status                     as status,
            datalens_usage.connection_type            as connection_type,
            datalens_usage.cache_used                 as cache_used,
            datalens_usage.is_public                  as is_public,
            datalens_usage.dataset_id                 as dataset_id,
            datalens_usage.source                     as source,
            datalens_usage.host                       as host,
            ba_cloud_folder.billing_account_id        as billing_account_id_folder,
            ba_cloud_org.billing_account_id           as billing_account_id_organization
        FROM $user_info_by_day as datalens_usage
        LEFT JOIN
            $ba_cloud_folder_dict_path as ba_cloud_folder
            ON datalens_usage.folder_id = ba_cloud_folder.folder_id
        LEFT JOIN
            $ba_cloud_organization_dict_path as ba_cloud_org
            ON datalens_usage.organization_id = ba_cloud_org.organization_id
        WHERE 
            (datalens_usage.folder_id is not NULL
                and datalens_usage.event_date >= SUBSTRING(ba_cloud_folder.cloud_start_msk, 0, 10)
                and datalens_usage.event_date < SUBSTRING(ba_cloud_folder.cloud_end_msk, 0, 10) 
            )
            OR
            (datalens_usage.organization_id is not NULL
                and datalens_usage.event_date >= SUBSTRING(ba_cloud_org.cloud_start_msk, 0, 10)
                and datalens_usage.event_date < SUBSTRING(ba_cloud_org.cloud_end_msk, 0, 10) 
            )
    );

-------RESULTS DL TABLES---------
    $result_dl_ba = (
        SELECT 
            event_date,
            cache_full_hit,
            folder_id,
            organization_id,
            dataset_mode,
            connection_id,
            user_id,
            total_execution_time,
            avg_execution_time,
            median_execution_time,
            request_count,
            status,
            connection_type,
            cache_used,
            is_public,
            dataset_id,
            source,
            host,
            CASE WHEN billing_account_id_folder is not NULL then billing_account_id_folder
                ELSE billing_account_id_organization END as billing_account_id
        FROM $join_user_ba

    );

    INSERT INTO $result_table_path WITH TRUNCATE
        SELECT *
        FROM $result_dl_ba;

    INSERT INTO $result_table_without_ba_path WITH TRUNCATE
        SELECT *
        FROM $user_info_raw;
        
END DEFINE;

EXPORT $datalens_daily_usage_script;
