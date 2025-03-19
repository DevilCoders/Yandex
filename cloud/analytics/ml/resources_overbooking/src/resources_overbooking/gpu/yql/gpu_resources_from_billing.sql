USE hahn;

$today = Datetime::Format('%Y-%m-%d')(CurrentTzDate('Europe/Moscow'));
$dm_yc_consumption_prod = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption';
$dm_yc_consumption_preprod = '//home/cloud-dwh/data/preprod/cdm/dm_yc_consumption';
$billing_accounts_prod = '//home/cloud-dwh/data/prod/ods/billing/billing_accounts';
$billing_accounts_preprod = '//home/cloud-dwh/data/preprod/ods/billing/billing_accounts';
$result_table = '//home/cloud_analytics/resources_overbooking/gpu/daily_table_from_yc';

$get_platform = ($sku_name) -> {
  Return
    CASE $sku_name
        WHEN 'compute.vm.gpu.gpu-standard' THEN 'gpu-standard-v1'
        WHEN 'compute.vm.gpu.gpu-standard.preemptible' THEN 'gpu-standard-v1'
        WHEN 'compute.vm.gpu.vgpu-standard.v1' THEN 'gpu-standard-v1'
        WHEN 'compute.vm.gpu.vgpu-standard.v1.preemptible' THEN 'gpu-standard-v1'
        WHEN 'compute.vm.gpu.gpu-standard.v2' THEN 'gpu-standard-v2'
        WHEN 'compute.vm.gpu.gpu-standard.preemptible.v2' THEN 'gpu-standard-v2'
        WHEN 'compute.vm.gpu.gpu-standard.v3' THEN 'gpu-standard-v3'
        WHEN 'compute.vm.gpu.gpu-standard.preemptible.v3' THEN 'gpu-standard-v3'
        WHEN 'compute.vm.gpu.vgpu-private.v3-5c' THEN 'gpu-standard-v3'
        WHEN 'compute.vm.gpu.standard.v3-t4' THEN 'standard-v3-t4'
        WHEN 'compute.vm.gpu.standard.v3-t4.preemptible' THEN 'standard-v3-t4'
        ELSE "undefined"
    END
};

$ba_statuses_prod =
    SELECT DISTINCT
        billing_account_id,
        usage_status,
        state
    FROM $billing_accounts_prod
;

$ba_statuses_preprod =
    SELECT DISTINCT
        billing_account_id,
        usage_status,
        state
    FROM $billing_accounts_preprod
;

$prod_table =
    SELECT
        'prod' AS env,
        yc.billing_record_msk_date AS billing_record_msk_date,
        yc.sku_name_rus AS sku_name_rus,
        yc.sku_name_eng AS sku_name_eng,
        yc.sku_name AS sku_name,
        bs.usage_status AS usage_status,
        bs.state AS state,
        $get_platform(yc.sku_name) AS platform,
        Sum(billing_record_pricing_quantity)/24 AS used_gpu
    FROM $dm_yc_consumption_prod AS yc
    LEFT JOIN $ba_statuses_prod AS bs USING(billing_account_id)
    WHERE sku_pricing_unit = 'gpu*hour'
        AND sku_service_name = 'compute'
        AND sku_name != 'compute.vm.gpu.gpu-e2e'
        AND billing_record_msk_date >= '2019-10-10'
        AND billing_record_msk_date < $today
    GROUP BY
        yc.billing_record_msk_date,
        yc.sku_name_rus,
        yc.sku_name_eng,
        yc.sku_name,
        bs.usage_status,
        bs.state
;

$preprod_table =
    SELECT
        'preprod' AS env,
        yc.billing_record_msk_date AS billing_record_msk_date,
        yc.sku_name_rus AS sku_name_rus,
        yc.sku_name_eng AS sku_name_eng,
        yc.sku_name AS sku_name,
        bs.usage_status AS usage_status,
        bs.state AS state,
        $get_platform(yc.sku_name) AS platform,
        Sum(billing_record_pricing_quantity)/24 AS used_gpu
    FROM $dm_yc_consumption_preprod AS yc
    LEFT JOIN $ba_statuses_preprod AS bs USING(billing_account_id)
    WHERE sku_pricing_unit = 'gpu*hour'
        AND sku_service_name = 'compute'
        AND sku_name != 'compute.vm.gpu.gpu-e2e'
        AND billing_record_msk_date >= '2019-10-10'
        AND billing_record_msk_date < $today
    GROUP BY
        yc.billing_record_msk_date,
        yc.sku_name_rus,
        yc.sku_name_eng,
        yc.sku_name,
        bs.usage_status,
        bs.state
;

INSERT INTO $result_table WITH TRUNCATE
    SELECT *
    FROM (
        SELECT * FROM $prod_table
        UNION ALL
        SELECT * FROM $preprod_table
    ) AS t
    WHERE usage_status != 'disabled'
    ORDER BY
        env,
        billing_record_msk_date,
        sku_name_rus,
        sku_name_eng,
        sku_name,
        usage_status,
        state,
        platform
;
