use hahn;

-- PRAGMA Library('time.sql');
IMPORT time SYMBOLS $format_date, $parse_date, $format_datetime, $parse_datetime;

--INSERT INTO `//home/cloud_analytics/ba_tags/ba_cloud_puid` WITH TRUNCATE  
DEFINE action $get_ba_cloud_puid($path) AS 
    INSERT INTO $path WITH TRUNCATE
    SELECT DISTINCT
        if(billing_account_id = 'dn23b9n6kk86tkldencb', NULL, billing_account_id) as billing_account_id, 
        effective_time, 
        ba_cloud.id as cloud_id, 
        passport_uid as puid,
        user_settings_email AS email,
        $format_date(DateTime::StartOfWeek(DateTime::ParseIso8601(cloud_created_at))) as cloud_cohort_w,
        $format_date(DateTime::StartOfMonth(DateTime::ParseIso8601(cloud_created_at))) as cloud_cohort_m
    FROM (SELECT * FROM `//home/cloud/billing/exported-billing-tables/clouds_prod`) as ba_cloud
    LEFT JOIN (SELECT * FROM `//home/cloud_analytics/import/iam/cloud_owners_history`) as cloud_puid
    ON ba_cloud.id = cloud_puid.cloud_id;
END DEFINE;

EXPORT $get_ba_cloud_puid;
