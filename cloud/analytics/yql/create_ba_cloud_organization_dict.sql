USE hahn;

$cloud_hist = '//home/cloud-dwh/data/prod/ods/iam/clouds_history';
$cloud_ba = '//home/cloud-dwh/data/prod/ods/billing/service_instance_bindings';

$result_table_path = '//home/cloud_analytics/dictionaries/ids/ba_cloud_organization_dict';

-- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
-- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
-- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$format_msk_date_by_timestamp = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", DateTime::Format("%Y-%m-%d %H:%M:%S")));


DEFINE ACTION $ba_cloud_organization_dict_script() AS

    $ba_cloud_org = (
        SELECT 
            cloud_id,
            organization_id,
            billing_account_id,
            $format_msk_date_by_timestamp(modified_at) as modified_at,
            $format_msk_date_by_timestamp(cloud_start_msk) as cloud_start_msk,
            $format_msk_date_by_timestamp(cloud_end_msk) as cloud_end_msk
        FROM $cloud_hist as ch
        JOIN (SELECT
                        service_instance_id,
                        billing_account_id,
                        start_time as cloud_start_msk,
                        end_time as cloud_end_msk
                    FROM $cloud_ba
                    WHERE 
                        `service_instance_type` = 'cloud'
                    ) as cba 
            ON ch.cloud_id = cba.service_instance_id
        WHERE 
            modified_at between cloud_start_msk and cloud_end_msk
        ORDER BY cloud, modified_at    
    );


    INSERT INTO $result_table_path WITH TRUNCATE
        SELECT *
        FROM $ba_cloud_org;

END DEFINE;

EXPORT $ba_cloud_organization_dict_script;
