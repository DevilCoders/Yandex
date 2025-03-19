INSERT INTO "<append=%false>{table}"
SELECT 
    d as date, 
    costs_type,
    costs,
    billing_account_id
FROM (
SELECT 
    billing_account_id,
    d,
    array('Hardware Costs Trial', 'Hardware Costs Paid') as costs_type,
    array(sum (trial_rub), sum (paid_rub)) as costs
FROM (
SELECT
    billing_account_id,
    d,
    service_name,
    subservice_name,
    pricing_unit_,
    trial_pr_q,
    paid_pr_q, 
    trial_pr_q * price as trial_rub,
    paid_pr_q * price as paid_rub
FROM (
SELECT
    billing_account_id,
    d,
    service_name,
    subservice_name,
    pricing_unit_,
    sum(trial_pr_q) as trial_pr_q,
    sum(paid_pr_q) as paid_pr_q
FROM (

SELECT
    billing_account_id,
    d,
    pricing_unit,
    pricing_unit_,
    sku_id,
    service_name,
    subservice_name,
    sku_name,
    --toDate(event_time) as d,
    
    
    
    pricing_quantity,
    if(trial+paid = 0, 0, pricing_quantity*trial / (trial+paid)) as trial_pr_q,
    if(trial+paid = 0, 0, pricing_quantity*paid / (trial+paid)) as paid_pr_q,
    trial,
    paid
FROM (
SELECT
    billing_account_id,
    d,
    pricing_unit,
    if(service_name = 'mdb' and pricing_unit ='hour', array('core*hour', 'gbyte*hour'), array(pricing_unit)) as pricing_unit_,
    sku_id,
    service_name,
    subservice_name,
    sku_name,
    --toDate(event_time) as d,
    
    
    
    multiIf(service_name = 'mdb' and pricing_unit ='hour', array(pricing_quantity*toFloat64(cores_number)*toFloat64(core_fraction_number)/100, pricing_quantity*toFloat64(ram_number)), 
            toFloat64(core_fraction_number)>1, array(pricing_quantity*toFloat64(core_fraction_number)/100), 
            array(pricing_quantity)) as pricing_quantity,
    trial,
    paid
FROM (
SELECT 
    billing_account_id,
    d,
    pricing_unit,
    sku_id,
    sku_name,
    --toDate(event_time) as d,
    sum(pricing_quantity) as pricing_quantity,
    sum(trial_consumption) as trial,
    sum(real_consumption) as paid
FROM (
SELECT 
    billing_account_id,
    toDate(event_time) as d,
    sku_id,
    sku_name,
    pricing_unit,
    event_time,
    --toDate(event_time) as d,
    pricing_quantity,
    trial_consumption,
    
    real_consumption
    
FROM (
    SELECT 
        billing_account_id,
        event_time,
        sku_id,
        name as sku_name,
        pricing_unit,
        pricing_quantity,
        real_consumption,
        trial_consumption
    FROM 
         "//home/cloud_analytics/cubes/acquisition_cube/cube"
) as a 
)
GROUP BY billing_account_id, pricing_unit, sku_id, sku_name,d
) AS c GLOBAL LEFT JOIN 
    "//home/cloud_analytics/export/billing/sku_tags/sku_tags" as d
ON c.sku_id = d.sku_id  
)
ARRAY JOIN pricing_unit_, pricing_quantity
)
GROUP BY 
    billing_account_id,
    d,
    service_name,
    subservice_name,
    pricing_unit_
) as a LEFT JOIN "//home/cloud_analytics/cubes/costs/pricing_unit_cost" as b 
ON a.service_name = b.service_name and a.subservice_name = b.subservice_name and a.pricing_unit_ = b.pricing_unit_
ORDER BY 
    d DESC,
    service_name,
    subservice_name 
) 
GROUP BY
billing_account_id,d
)
ARRAY JOIN costs_type, costs
WHERE costs > 0 