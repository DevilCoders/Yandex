use hahn;


IMPORT time SYMBOLS $format_date, $parse_date;
$destination_table = "//home/cloud_analytics/cubes/consumption_by_folders/consumption_by_folders";

DEFINE SUBQUERY $last_half_year_consumption() AS
SELECT
    billing_record_credit_rub,
    billing_account_balance_name_at_moment,
    sku_tag_service_group,
    billing_record_cloud_id AS cloud_id,
    billing_record_folder_id ?? 'none'  as folder_id,
    --folder_name,
    sku_tag_service_name,
    billing_account_name,
    billing_record_cost_rub,
    sku_name,
    sku_tag_subservice_name,
    billing_account_sales_name,
    billing_record_date,
    billing_account_company_name,
    billing_account_id,
    (billing_record_cost_rub + billing_record_credit_rub) / 1.2 as paid_consumption_vat,
    - billing_record_credit_rub / 1.2 as trial_consumption_vat
    
FROM `//home/cloud/billing/analytics_cube/realtime/prod`
WHERE billing_record_date >= $format_date(DateTime::StartOfMonth(YQL::CurrentUtcDate() - INTERVAL ('P26W')))
END DEFINE;

DEFINE SUBQUERY $consumption_by_folder() AS
SELECT
    cons.billing_account_balance_name_at_moment as billing_account_balance_name_at_moment,
    cons.sku_tag_service_group as sku_tag_service_group,
    cons.cloud_id as cloud_id,
    cons.folder_id as folder_id,
    folders.folder_name as folder_name,
    cons.sku_tag_service_name as sku_tag_service_name,
    cons.billing_account_name as billing_account_name,
    cons.sku_name as sku_name,
    cons.sku_tag_subservice_name as sku_tag_subservice_name,
    cons.billing_account_sales_name as billing_account_sales_name,
    cons.billing_record_date as billing_record_date,
    cons.billing_account_company_name as billing_account_company_name,
    cons.billing_account_id as billing_account_id,
    sum(cons.paid_consumption_vat) as paid_consumption_vat,
    sum(cons.trial_consumption_vat) as trial_consumption_vat
FROM $last_half_year_consumption() as cons
LEFT JOIN `//home/cloud_analytics/import/iam/cloud_folders/1h/latest` as folders
ON cons.folder_id = folders.folder_id

GROUP BY
    cons.billing_account_balance_name_at_moment,
    cons.sku_tag_service_group,
    cons.cloud_id,
    cons.folder_id,
    folders.folder_name,
    cons.sku_tag_service_name,
    cons.billing_account_name,
    cons.sku_name,
    cons.sku_tag_subservice_name,
    cons.billing_account_sales_name,
    cons.billing_record_date,
    cons.billing_account_company_name,
    cons.billing_account_id
ORDER BY billing_record_date DESC
END DEFINE;

EXPORT $destination_table, $consumption_by_folder; 


