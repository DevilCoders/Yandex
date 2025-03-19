INSERT INTO "<append=%false>//home/cloud_analytics/cubes/costs/costs_telesales"
SELECT
    toString(t0.date) as date,
    'telesales_costs' as  costs_type,
    SUM(t1.costs) as costs,
    t0.billing_account_id as billing_account_id
FROM(
    SELECT
        toDate(event_time) as date,
        sales_name,
        billing_account_id
    FROM "//home/cloud_analytics_test/cubes/crm_leads/cube"
    WHERE
        event = 'call'
        AND billing_account_id like 'dn2%'
) as t0
ALL INNER JOIN(
    SELECT
        date,
        sales_name,
        costs/cnt  as costs
    FROM(
        SELECT
            toDate(event_time) as date,
            sales_name,
            CoUNT(*) as cnt,
            {r}*8*247/365 as costs
        FROM "//home/cloud_analytics_test/cubes/crm_leads/cube"
        WHERE
            event = 'call'
            AND billing_account_id like 'dn2%'
        GROUP BY
            date,
            sales_name,
            costs
    )
) as t1
ON t0.sales_name = t1.sales_name AND t0.date = t1.date
GROUP BY
    billing_account_id,
    date,
    costs_type
HAVING
    costs > 0
