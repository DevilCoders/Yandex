INSERT INTO "<append=%false>{table}"
SELECT
    toString(ba.date) as date,
    'Marketing Perfomance' as costs_type,
    SUM(costs.costs) as costs,
    ba.billing_account_id as billing_account_id
FROM(
    SELECT
        toDate(event_time) as date,
        billing_account_id
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE
        event = 'ba_created'
) as ba
SEMI LEFT JOIN(
    SELECT
        t0.date,
        t1.costs / t0.bas as costs
    FROM(
        SELECT
            toDate(event_time) as date,
            COUNT(DISTINCT billing_account_id) as bas
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'ba_created'
        GROUP BY
            date
    )  as  t0
     SEMI LEFT JOIN(
        SELECT
            toDate(date)  as date,
            SUM(cost) as costs
        FROM "//home/cloud_analytics/import/marketing/ad_costs/ad_costs"
        GROUP BY
            date
    ) as t1
    ON t0.date = t1.date
) as costs
ON ba.date = costs.date
GROUP BY
    billing_account_id,
    date,
    costs_type
HAVING
    costs > 0
