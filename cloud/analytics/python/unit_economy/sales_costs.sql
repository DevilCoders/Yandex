INSERT INTO "<append=%false>//home/cloud_analytics/cubes/costs/costs_sales"
SELECT
    toString(t0.date) as date,
    'sales_costs' as costs_type,
    t1.sales_costs as costs,
     t0.billing_account_id as billing_account_id
FROM(
    SELECT
        *
    FROM "//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/ba_hist"
    WHERE
        sales_name != 'unmanaged'
) as t0
ALL INNER JOIN(
    SELECT
        date,
        sales_name,
        costs/bas as sales_costs
    FROM(
        SELECT
            date as date,
            sales_name,
            multiIf(
                sales_name IN ('golubin', 'datishin', 'sergeykn', 'alexche', 'andreigusev') AND toDayOfWeek(toDate(date)) < 6, {r1}*8*247/365,
                sales_name IN ('ebelobrova', 'niktk', 'glebmarkevich', 'kalifornia', 'marinapolik', 'victorbutenko') AND toDayOfWeek(toDate(date)) < 6, {r2}*8*247/365,
                0.0
            ) as costs,
            COUNT(DISTINCT billing_account_id) as bas
        FROM "//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/ba_hist"
        WHERE
            sales_name != 'unmanaged'
        GROUP BY
            date,
            sales_name,
            costs
    )
) as t1
ON t0.sales_name = t1.sales_name AND t0.date = t1.date
WHERE
    sales_costs > 0