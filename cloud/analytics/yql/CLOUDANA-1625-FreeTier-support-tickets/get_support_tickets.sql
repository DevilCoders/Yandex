use hahn;

DEFINE SUBQUERY $get_number_of_tickets() AS

    $get_issues = (
        SELECT
            components.startrek_key AS startrek_key,
            issues.billing_id AS billing_account_id,
            DateTime::StartOfMonth(issues.created_at) AS month,
            components.name AS name
        FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_issue_components` AS components 
        INNER JOIN `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues` AS issues ON issues.startrek_key = components.startrek_key
        WHERE 
            issues.billing_id !='None'
            );

    $consumption = (
        SELECT 
            DateTime::StartOfMonth(DateTime::ParseIso8601(event_time)) as usage_month,
            billing_account_id,
            service_name,
            subservice_name,
            sum(br_pricing_quantity),
            sum(br_cost),
            sum(br_total)
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE
            event_time>'2021-01-01'
        GROUP BY
            event_time,
            billing_account_id,
            service_name,
            subservice_name
        HAVING
            sum(br_pricing_quantity) > 0 
            AND  sum(br_cost) = 0);

    --tickets by tag
    SELECT
        DateTime::ToMilliseconds(issues.month)  AS month,
        issues.name AS name,
        count(distinct issues.startrek_key) AS num_of_tickets,
    FROM $get_issues AS issues 
    INNER JOIN  $consumption AS consumption ON issues.billing_account_id = consumption.billing_account_id
    WHERE 
        (issues.name = consumption.service_name OR 
        issues.name = consumption.subservice_name)
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
        (issues.name = consumption.service_name OR 
        issues.name = consumption.subservice_name)
        AND issues.month = consumption.usage_month
    GROUP BY 
        issues.month;    
END DEFINE;

EXPORT $get_number_of_tickets;