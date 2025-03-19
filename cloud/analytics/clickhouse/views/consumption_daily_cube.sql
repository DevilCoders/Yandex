CREATE TABLE cloud_analytics.consumption_daily_cube ENGINE = MergeTree() ORDER BY(date) PARTITION BY toYYYYMM(date) 
AS 
SELECT 
    event_time as date,
    toDate(event_time) as date_date,
    assumeNotNull(billing_account_id) as billing_account_id,
    assumeNotNull(cloud_id) as cloud_id,
    assumeNotNull(service_name) as service_name,
    assumeNotNull(subservice_name) as subservice_name,
    assumeNotNull(core_fraction) as core_fraction,
    assumeNotNull(preemptible) as preemptible,
    assumeNotNull(platform) as platform,
    assumeNotNull(sku_name) as sku_name,
    assumeNotNull(block_reason) as block_reason,
    assumeNotNull(real_consumption) as cons_sum,
    'paid' as cons_type,
    assumeNotNull(ba_state) as state,
    assumeNotNull(multiIf(account_name not in ('','unknown'), account_name, balance_name != '', balance_name, ba_name)) as client_name,
    assumeNotNull(last_client_name) as last_client_name,
    --if(assumeNotNull(segment) = '', 'mass', segment) as segment,
    last_segment as segment,
    assumeNotNull(potential) as potential,
    assumeNotNull(last_sales_name) as last_sales_name,
    assumeNotNull(last_state) as last_state,
    assumeNotNull(last_block_reason) as last_block_reason,
    assumeNotNull(sales_name) as sales_name,
    assumeNotNull(architect) as architect,
    assumeNotNull(last_architect) as last_architect,
    master_account_id,
    assumeNotNull(database) as database,
    assumeNotNull(ba_person_type) as person_type,
    assumeNotNull(first_ba_created_datetime) as created_at,
    assumeNotNull(channel) as channel,
    assumeNotNull(ba_usage_status) as usage_status,
    assumeNotNull(total) as total
FROM cloud_analytics.acquisition_cube
ANY LEFT JOIN (
    SELECT
        billing_account_id,
        argMax(sales_name, event_time) as last_sales_name,
        argMax(architect, event_time) as last_architect,
        argMax(multiIf(account_name not in ('','unknown'), account_name, balance_name != '', balance_name, ba_name), event_time) as last_client_name,
        argMax(if(assumeNotNull(segment) = '' or billing_account_id  in (
                                'dn23tf22je8ehdv1f4d5', --  polievkt.check polievkt.check polievkt.check
                                'dn2sd5bi5tjktnea54ce', --  ptest-isv,
                                'dn2c54osc9plknji7msd' -- wrong_isv
                                ), 'mass', segment), event_time) as last_segment,
        argMax(ba_state, event_time) as last_state,
        argMax(block_reason, event_time) as last_block_reason
    FROM cloud_analytics.acquisition_cube
    WHERE event = 'day_use'
    GROUP BY billing_account_id
)
USING billing_account_id
WHERE (event = 'day_use') 
and (real_consumption>0) 

UNION ALL 

SELECT 
    event_time as date,
    toDate(event_time) as date_date,
    assumeNotNull(billing_account_id) as billing_account_id,
    assumeNotNull(cloud_id) as cloud_id,
    assumeNotNull(service_name) as service_name,
    assumeNotNull(subservice_name) as subservice_name,
    assumeNotNull(core_fraction) as core_fraction,
    assumeNotNull(preemptible) as preemptible,
    assumeNotNull(platform) as platform,
    assumeNotNull(sku_name) as sku_name,
    assumeNotNull(block_reason) as block_reason,
    assumeNotNull(trial_consumption) as cons_sum,
    'trial' as cons_type,
    assumeNotNull(ba_state) as state,
    assumeNotNull(multiIf(account_name not in ('','unknown'), account_name, balance_name != '', balance_name, ba_name)) as client_name,
    assumeNotNull(last_client_name) as last_client_name,
    --if(assumeNotNull(segment) = '', 'mass', segment) as segment,
    last_segment as segment,
    assumeNotNull(potential) as potential,
    assumeNotNull(last_sales_name) as last_sales_name,
    assumeNotNull(last_state) as last_state,
    assumeNotNull(last_block_reason) as last_block_reason,
    assumeNotNull(sales_name) as sales_name,
    assumeNotNull(architect) as architect,
    assumeNotNull(last_architect) as last_architect,
    master_account_id,
    assumeNotNull(database) as database,
    assumeNotNull(ba_person_type) as person_type,
    assumeNotNull(first_ba_created_datetime) as created_at,
    assumeNotNull(channel) as channel,
    assumeNotNull(ba_usage_status) as usage_status,
    assumeNotNull(total) as total
FROM cloud_analytics.acquisition_cube
ANY LEFT JOIN (
    SELECT
        billing_account_id,
        argMax(sales_name, event_time) as last_sales_name,
        argMax(architect, event_time) as last_architect,
        argMax(multiIf(account_name not in ('','unknown'), account_name, balance_name != '', balance_name, ba_name), event_time) as last_client_name,
        argMax(if(assumeNotNull(segment) = '' or billing_account_id  in (
                                'dn23tf22je8ehdv1f4d5', --  polievkt.check polievkt.check polievkt.check
                                'dn2sd5bi5tjktnea54ce', --  ptest-isv,
                                'dn2c54osc9plknji7msd' -- wrong_isv
                                ), 'mass', segment), event_time) as last_segment,
        argMax(ba_state, event_time) as last_state,
        argMax(block_reason, event_time) as last_block_reason
    FROM cloud_analytics.acquisition_cube
    WHERE event = 'day_use'
    GROUP BY billing_account_id
        )
USING billing_account_id
WHERE (event = 'day_use') 
and (trial_consumption>0) 


UNION ALL

SELECT 
    event_time as date,
    toDate(event_time) as date_date,
    assumeNotNull(billing_account_id) as billing_account_id,
    assumeNotNull(cloud_id) as cloud_id,
    'compute' as service_name,
    'cpu' as subservice_name,
    assumeNotNull(core_fraction) as core_fraction,
    assumeNotNull(preemptible) as preemptible,
    assumeNotNull(platform) as platform,
    '' as sku_name,
    assumeNotNull(block_reason) as block_reason,
    assumeNotNull(real_consumption) as cons_sum,
    'trial' as cons_type,
    assumeNotNull(ba_state) as state,
    assumeNotNull(multiIf(account_name not in ('','unknown'), account_name, balance_name != '', balance_name, ba_name)) as client_name,
    assumeNotNull(last_client_name) as last_client_name,
    --if(assumeNotNull(segment) = '', 'mass', segment) as segment,
    last_segment as segment,
    assumeNotNull(potential) as potential,
    assumeNotNull(last_sales_name) as last_sales_name,
    assumeNotNull(last_state) as last_state,
    assumeNotNull(last_block_reason) as last_block_reason,
    assumeNotNull(sales_name) as sales_name,
    assumeNotNull(architect) as architect,
    assumeNotNull(last_architect) as last_architect,
    master_account_id,
    assumeNotNull(database) as database,
    assumeNotNull(ba_person_type) as person_type,
    assumeNotNull(first_ba_created_datetime) as created_at,
    assumeNotNull(channel) as channel,
    assumeNotNull(ba_usage_status) as usage_status,
    assumeNotNull(total) as total
FROM cloud_analytics.acquisition_cube
ANY LEFT JOIN (
    SELECT
        billing_account_id,
        argMax(sales_name, event_time) as last_sales_name,
        argMax(architect, event_time) as last_architect,
        argMax(multiIf(account_name not in ('','unknown'), account_name, balance_name != '', balance_name, ba_name), event_time) as last_client_name,
        argMax(if(assumeNotNull(segment) = '' or billing_account_id  in (
                                'dn23tf22je8ehdv1f4d5', --  polievkt.check polievkt.check polievkt.check
                                'dn2sd5bi5tjktnea54ce', --  ptest-isv,
                                'dn2c54osc9plknji7msd' -- wrong_isv
                                ), 'mass', segment), event_time) as last_segment,
        argMax(ba_state, event_time) as last_state,
        argMax(block_reason, event_time) as last_block_reason
    FROM cloud_analytics.acquisition_cube
    WHERE event = 'day_use'
    GROUP BY billing_account_id
        )
USING billing_account_id
WHERE (event = 'ba_created') 