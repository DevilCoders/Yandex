INSERT INTO "<append=%false>//home/cloud_analytics/unit_economy/architect_costs/architect_costs"
SELECT 
    d as date,
    'architects' as costs_type,
    coalesce(costs, 0) as costs,
    billing_account_id
FROM (
SELECT 
    d,
    billing_account_id,
    segment,
    costs/ba_count as costs
FROM (
SELECT
    d,
    billing_account_id,
    {r1} * arch_count * 247/365*8 as arch_costs,
    segment,
    multiIf(segment='Enterprise', arch_costs*0.44, segment='Medium', arch_costs*0.25, segment='Mass', arch_costs*0.04, segment='ISV Program', arch_costs*0.19, segment='VAR', arch_costs*0.08, 0 ) as costs
FROM
    (
    SELECT DISTINCT
        billing_account_id,
        toDate(event_time) as d,
        segment,
        staff_count as arch_count
    FROM 
    "//home/cloud_analytics/cubes/acquisition_cube/cube" as a GLOBAL LEFT JOIN (
        SELECT d, staff_count
            FROM (
            SELECT groupArray(date) as d, arrayCumSum(groupArray(staff_count)) as staff_count
            FROM (
            SELECT date
            FROM (
            SELECT arrayMap(x->addDays(toDate('2018-01-01'), x),range(toUInt64(dateDiff('day',  toDate('2018-01-01'), now()))+1)) as date
            )
            ARRAY JOIN date
            ) as a left join (
            SELECT
                toDate(date_) as date, staff_count
            FROM (
            SELECT
                groupArray(join_at) as date_,
                groupArray(c) as staff_count
            FROM (
            SELECT
                if(join_at<'2018-01-01', '2018-01-01', join_at) as join_at, count(*) as c
            FROM "//home/cloud_analytics/import/staff/cloud_staff/cloud_staff"
            WHERE group_url = 'yandex_exp_9053_4245_4897'
            GROUP BY join_at
            ORDER BY join_at 
            )
            )
            ARRAY JOIN date_, staff_count
            ) as b 
            USING date
            ) 
            ARRAY JOIN d, staff_count
    ) as b 
    USING d
    WHERE assumeNotNull(architect) != 'no_architect' and event = 'day_use'
    )
) as a GLOBAL LEFT JOIN (
    SELECT 
        toDate(event_time) as d,
        segment,
        count(DISTINCT billing_account_id) as ba_count 
    FROM
        "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE assumeNotNull(architect) != 'no_architect' and event = 'day_use'
    GROUP BY d, segment 
    )
USING d, segment
)
WHERE costs >0
