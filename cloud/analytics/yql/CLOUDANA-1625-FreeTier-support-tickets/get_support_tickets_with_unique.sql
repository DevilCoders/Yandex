use hahn;

DEFINE SUBQUERY $get_number_of_tickets() AS
    $parse1 = DateTime::Parse("%Y-%m-%d");

    $get_issues = (
        SELECT
            components.issue_key AS startrek_key,
            issues.billing_account_id AS billing_account_id,
            DateTime::StartOfMonth(issues.created_at) AS month,
            components.component_name AS name
        FROM `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_component_issues` AS components 
        INNER JOIN (select * from `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_issues`) AS issues ON issues.issue_key = components.issue_key
        WHERE 
            issues.billing_account_id !=''
            )
        ;

    $consumption = (
        SELECT 
            $parse1(billing_record_msk_month) as usage_month,
            billing_account_id,
            sku_service_name,
            sku_subservice_name,
            sum(billing_record_pricing_quantity),
            sum(billing_record_cost),
            sum(billing_record_total)
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
        WHERE
            billing_record_msk_date>'2021-01-01'
        GROUP BY
            billing_record_msk_month,
            billing_account_id,
            sku_service_name,
            sku_subservice_name
        HAVING
            sum(billing_record_pricing_quantity) > 0 
            AND  sum(billing_record_cost) = 0);

    --tickets by tag
    SELECT
        DateTime::ToMilliseconds(issues.month)  AS month,
        issues.name AS name,
        count(distinct issues.startrek_key) AS num_of_tickets,
    FROM $get_issues AS issues 
    INNER JOIN  $consumption AS consumption ON issues.billing_account_id = consumption.billing_account_id
    WHERE 
        (issues.name = consumption.sku_service_name OR 
        issues.name = consumption.sku_subservice_name)
        AND issues.month = consumption.usage_month
    GROUP BY 
        issues.month,
        issues.name
    --unique tickets by month
    UNION ALL
        SELECT
        DateTime::ToMilliseconds(issues.month)  AS month,
        'all_tags' AS name,
        count(distinct issues.startrek_key) AS num_of_tickets,
    FROM $get_issues AS issues 
    INNER JOIN  $consumption AS consumption ON issues.billing_account_id = consumption.billing_account_id
    WHERE 
        (issues.name = consumption.sku_service_name OR 
        issues.name = consumption.sku_subservice_name)
        AND issues.month = consumption.usage_month
    GROUP BY 
        issues.month;    
END DEFINE;

EXPORT $get_number_of_tickets;