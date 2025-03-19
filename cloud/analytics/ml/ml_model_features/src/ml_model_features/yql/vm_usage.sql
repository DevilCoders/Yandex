Use hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA OrderedColumns;
DECLARE $dates AS List<String>;
    
$dm_crm_tags_path = "//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags";
$dm_vm_cube_path = "//home/cloud-dwh/data/prod/cdm/yc_vm_usage/yc_vm_usage/1d";


DEFINE ACTION $get_data_for_one_date($date) as
    $output = "//home/cloud_analytics/ml/ml_model_features/by_baid/vm_usage" || "/" || $date;
    $pattern = $date || "-%";

    $spdf_vm_cube_base = (
    SELECT
        `billing_account_id`,
        `vm_id`,
        TableName() as `date`,
        `vm_cores` as `cores`,
        `vm_gpus` as `gpus`,
        `vm_memory` as `memory`,
        `vm_public_fips` as `vm_public_fips`
    FROM RANGE($dm_vm_cube_path, $date || '-01' , $date || '-31')
    );

    $spdf_vm_cube = (
    SELECT 
        `billing_account_id`,
        `date`,
        COALESCE(SUM(`cores`)) as `vm_cores_sum`,
        COALESCE(AVG(`cores`)) as `vm_cores_avg`,
        COALESCE(MIN(`cores`)) as `vm_cores_min`,
        COALESCE(MAX(`cores`)) as `vm_cores_max`,
        COALESCE(SUM(`gpus`)) as `vm_gpus_sum`,
        COALESCE(AVG(`gpus`)) as `vm_gpus_avg`,
        COALESCE(MIN(`gpus`)) as `vm_gpus_min`,
        COALESCE(MAX(`gpus`)) as `vm_gpus_max`,
        COALESCE(SUM(`memory`)) as `vm_memory_sum`,
        COALESCE(AVG(`memory`)) as `vm_memory_avg`,
        COALESCE(MIN(`memory`)) as `vm_memory_min`,
        COALESCE(MAX(`memory`)) as `vm_memory_max`,
        COALESCE(SUM(`vm_public_fips`)) as `vm_public_fips_sum`,
        COALESCE(AVG(`vm_public_fips`)) as `vm_public_fips_avg`,
        COALESCE(MIN(`vm_public_fips`)) as `vm_public_fips_min`,
        COALESCE(MAX(`vm_public_fips`)) as `vm_public_fips_max`,
        COALESCE(COUNT(`vm_id`)) as `vm_count`
    FROM $spdf_vm_cube_base
    GROUP BY `billing_account_id`, `date`
    );

    INSERT INTO $output WITH TRUNCATE
    SELECT * 
    FROM (SELECT `billing_account_id`, `date` FROM $dm_crm_tags_path WHERE `date` LIKE $pattern) as x
    LEFT JOIN $spdf_vm_cube as y
    ON x.`billing_account_id` == y.`billing_account_id` AND x.`date` == y.`date`
END DEFINE;

EVALUATE FOR $date IN $dates
    DO $get_data_for_one_date($date)