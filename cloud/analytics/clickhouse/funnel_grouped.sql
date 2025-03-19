CREATE VIEW cloud_analytics.funnel_grouped (
week String,
month String,
segment String,
cloud_status String,
ba_curr_state String,
sales_person String,
source String,
source_detailed String,
cloud_created UInt64,
ba_created UInt64,
first_consumption UInt64,
ba_paid UInt64,
first_paid_consumption UInt64
) AS (

SELECT 
    week,
    month,
    lower(segment) as segment,
    cloud_status,
    ba_curr_state,
    sales_person,
    source,
    source_detailed,
    sum(step1_condition) as cloud_created,
    sum(step2_condition) as ba_created,
    sum(step3_condition) as first_consumption,
    sum(step4_condition) as ba_paid,
    sum(step5_condition) as first_paid_consumption
FROM(
    SELECT 
    passport_uid, 
    week,
    month,
    segment,
    cloud_status,
    ba_curr_state,
    sales_person,
    source,
    source_detailed,
    max(event_type='cloud_created') as step1_condition,
    max(event_type='ba_created') as step2_condition,
    max(event_type='ba_first_consumption') as step3_condition,
    max(event_type='ba_paid') as step4_condition,
    max(event_type='ba_first_paid_consumption') as step5_condition
    FROM cloud_analytics.funnel_events
    --WHERE is_fake='no'
    GROUP BY 
        passport_uid, 
        week,
        month,
        segment,
        cloud_status,
        ba_curr_state,
        sales_person,
        source,
        source_detailed
    )
GROUP BY
    week,
    month,
    segment,
    cloud_status,
    ba_curr_state,
    sales_person,
    source,
    source_detailed
    
ORDER BY
    week DESC
    
)
    