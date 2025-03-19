INSERT INTO `//home/cloud_analytics/cubes/costs/costs_events` WITH TRUNCATE 
SELECT 
    event_date as `date`,
    'event_costs' as costs_type,
    costs as costs,
    billing_account_id
FROM (

--по списку ба
SELECT 
    event_date, billing_account_id,
    sum(costs) as costs
    
FROM (
SELECT
    event_date,
    event_costs/ba_count as costs,
    ba_list as billing_account_id
FROM (
SELECT 
    event_id,
    event_date,
    event_costs,
    count(billing_account_id) as ba_count,
    AGGREGATE_LIST(billing_account_id) as ba_list
FROM (
SELECT
    event_id,
    event_date,
    event_costs,
    event_segment,
    mailing_id,
    distr_by_segment_only,
    b.billing_account_id as billing_account_id,
    if(segment in ('Enterprise', 'Large ISV'), 'Ent', if(segment in ('ISV ML', 'ISV Program'),'ISV', if(segment = 'VAR', 'VAR', 'SMB'))) as ba_segment
FROM (
SELECT 
    event_id,
    event_date,
    event_costs,
    event_segment,
    mailing_id,
    distr_by_segment_only
FROM (
SELECT  
    event_id,
    event_date,
    event_costs,
    event_segment,
    distr_by_segment_only,
    Yson::ConvertToStringList(mailing_id) as mailing_id
FROM `//home/cloud_analytics/events/events_costs`
WHERE distribute_by = 'mailing' --and distr_by_segment_only = 1
)
FLATTEN BY mailing_id ) as a
INNER JOIN (
    SELECT distinct 
        mail_id,
        billing_account_id
    FROM 
        `//home/cloud_analytics_test/cubes/emailing/cube`
    ) as b 
ON a.mailing_id = b.mail_id
INNER JOIN (
    SELECT
        billing_account_id,
        MAX_BY(segment, event_time) as segment
    FROM
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
    GROUP BY  
        billing_account_id
    
) as c 
ON b.billing_account_id = c.billing_account_id
)
WHERE ((distr_by_segment_only=0) OR ((distr_by_segment_only=1) AND (event_segment = ba_segment))) 
GROUP BY 
 event_id,
    event_date,
    event_costs
)
FLATTEN BY ba_list
)
GROUP BY 
billing_account_id, event_date

UNION ALL 
--по ид ба
SELECT  
    event_date, billing_account_id,
    sum(event_costs) as costs
FROM (
SELECT 
    event_id,
    event_date,
    event_costs,
    a.billing_account_id as billing_account_id
FROM (
SELECT  
    event_id,
    event_date,
    event_costs/ba_count as event_costs,
    billing_account_id
FROM (
SELECT  
    event_id,
    event_date,
    event_costs,
    Yson::ConvertToStringList(ba_list) as billing_account_id,
    ListLength(Yson::ConvertToStringList(ba_list)) as ba_count
FROM `//home/cloud_analytics/events/events_costs`
WHERE distribute_by = 'billing_account' --and distr_by_segment_only = 1
)
FLATTEN BY billing_account_id ) as a
INNER JOIN (
    SELECT
        billing_account_id,
        MAX_BY(segment, event_time) as segment
    FROM
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
    GROUP BY  
        billing_account_id
    
) as c 
ON a.billing_account_id = c.billing_account_id
)
GROUP BY 
    event_date, billing_account_id
    
    
UNION ALL
--по сегменту
SELECT 
    event_date, billing_account_id,
    sum(event_costs) as costs
FROM (

SELECT
    event_date,
    event_costs,
    billing_account_id
FROM (
SELECT 
    event_id,
    event_date,
    event_costs/ListLength(billing_account_id) as event_costs,
    a.event_segment as event_segment,
    billing_account_id
FROM (
SELECT  
    event_id,
    event_date,
    event_costs,
    event_segment, 
    
FROM `//home/cloud_analytics/events/events_costs`
WHERE distribute_by = 'segment' ) as a
INNER JOIN (
    SELECT
         event_segment,
        AGGREGATE_LIST(billing_account_id) as billing_account_id
    FROM (
    SELECT
        billing_account_id,
        if(MAX_BY(segment, event_time) in ('Enterprise', 'Large ISV'), 'Ent', if(MAX_BY(segment, event_time) in ('ISV ML', 'ISV Program'),'ISV', if(MAX_BY(segment, event_time) = 'VAR', 'VAR', 'SMB'))) as event_segment
    FROM
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
    GROUP BY  
        billing_account_id
    ) 
    GROUP BY 
        event_segment
    ) as b 
ON a.event_segment = b.event_segment
)
FLATTEN BY billing_account_id
)
GROUP BY
event_date, billing_account_id
)