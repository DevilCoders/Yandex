USE hahn;


-- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
-- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
-- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$format_msk_datetime_by_timestamp = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", DateTime::Format("%Y-%m-%d %H:%M:%S")));


$folders_table = "//home/cloud-dwh/data/prod/ods/iam/folders_history";
$service_instances_table = "//home/cloud-dwh/data/prod/ods/billing/service_instance_bindings";

$result_path = "//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict_new";

DEFINE ACTION $ba_cloud_folder_dict_script() AS
-- get clouds
    $clouds = (
        SELECT DISTINCT
            service_instance_id AS cloud_id,
            billing_account_id,
            $format_msk_datetime_by_timestamp(start_time) AS cloud_start_msk,
            $format_msk_datetime_by_timestamp(end_time) AS cloud_end_msk
        FROM $service_instances_table
        WHERE service_instance_type = "cloud"
    );

    -- get folders with active status
    $folders = (
        SELECT DISTINCT
            folder_id,
            cloud_id
        FROM $folders_table
        WHERE status = "ACTIVE"
    );

    -- create result table
    $result = (
        SELECT DISTINCT 
            folders.folder_id as folder_id,
            folders.cloud_id as cloud_id,
            clouds.billing_account_id as billing_account_id,
            clouds.cloud_start_msk AS cloud_start_msk,
            clouds.cloud_end_msk AS cloud_end_msk
        FROM $folders AS folders
        LEFT JOIN $clouds AS clouds USING(cloud_id)
    );


    INSERT INTO $result_path 
    WITH TRUNCATE
        SELECT 
            folder_id,
            cloud_id,
            billing_account_id,
            cloud_start_msk,
            cloud_end_msk
        FROM $result
        ORDER BY folder_id, cloud_id, cloud_start_msk

END DEFINE;

EXPORT $ba_cloud_folder_dict_script;
