USE hahn;

IMPORT time SYMBOLS $format_date, $parse_datetime;

-- SELECT $format_date(CurrentUtcDate());
DEFINE ACTION $dim_unique_values($dim, $destination_table) AS
    INSERT INTO $destination_table
    SELECT DISTINCT 
        CASE $dim 
        WHEN 'monthly_cohort' THEN SUBSTRING($format_date(DateTime::StartOfMonth($parse_datetime(first_ba_created_datetime))),0,7)
        ELSE CAST(TableRow().$dim AS String)
        END
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`;
END DEFINE;
    
DEFINE ACTION $cube_unique($destination_table, $destination_table_2) AS

    
    EVALUATE FOR $dim in AsList(
       'segment',
        'sales_name',
        'service_group',
        'service_name',
        'subservice_name',
        'ba_person_type',
        'is_fraud',
        'crm_account_segment',
        'crm_account_owner',
        'crm_account_architect',
        'crm_account_bus_dev',
        'crm_account_sales',
        'crm_account_partner_manager',
        -- 'account_name'
        -- 'monthly_cohort'
        -- 'marketing_touched_events_count_bucket'
        )
    DO $dim_unique_values($dim, $destination_table);
    
    --$destination_table = $destination_folder || $destination_file;
    INSERT INTO $destination_table
    SELECT DISTINCT
    $format_date(DateTime::StartOfMonth($parse_datetime(first_ba_created_datetime)))
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`;
    
    INSERT INTO $destination_table
    SELECT DISTINCT
    account_name || ' (' || billing_account_id || ')' as column0
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE crm_account_tags like '%2620ade2-2d64-11eb-be5b-2ace7971c770%';
    
    INSERT INTO $destination_table
    SELECT DISTINCT
    account_name as column0
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE crm_account_tags like '%2620ade2-2d64-11eb-be5b-2ace7971c770%';
    
    INSERT INTO $destination_table_2
    SELECT DISTINCT
        marketing_attribution_channel,
        marketing_attribution_utm_campaign,
        marketing_attribution_utm_source,
        marketing_attribution_utm_medium,
        marketing_attribution_utm_term
    FROM
       `//home/cloud_analytics/cubes/acquisition_cube/cube_with_marketing_attribution`
END DEFINE;

EXPORT $cube_unique;
