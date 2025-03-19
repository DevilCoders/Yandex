use hahn;


IMPORT time SYMBOLS $parse_date, $parse_datetime, $format_date;


DEFINE subquery $result() AS 
    $ba_cohort_m = (
        SELECT 
            billing_account_id, 
            $format_date(DateTime::StartOfMonth($parse_date(event_date))) as ba_cohort_m,
            $format_date(DateTime::StartOfWeek($parse_date(event_date))) as ba_cohort_w,
            event_time as ba_created_datetime
        FROM `//home/cloud_analytics/marketing/events_for_attribution`
        WHERE event_type = 'ba_created'
        );
        
    $ba_channel = (
        SELECT 
            billing_account_id, 
            marketing_attribution_event_id as event_id,
            marketing_attribution_event_exp_7d_half_life_time_decay_weight,
            marketing_attribution_event_first_weight,
            marketing_attribution_event_last_weight,
            marketing_attribution_event_uniform_weight,
            marketing_attribution_event_u_shape_weight,
            coalesce(marketing_attribution_channel, 'Unknown channel') as channel,
            coalesce(marketing_attribution_channel_marketing_influenced, 'Unknown channel') as channel_marketing_influenced,
            coalesce(String::ToLower(marketing_attribution_utm_source), 'Unknown utm_source') as utm_source,
            coalesce(String::ToLower(marketing_attribution_utm_campaign), 'Unknown utm_campaign') as utm_campaign,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name), 'Unknown utm_campaign_name') as utm_campaign_name,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_1), 'Unknown utm_campaign_name_1') as utm_campaign_name_1,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_2), 'Unknown utm_campaign_name_2') as utm_campaign_name_2,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_3), 'Unknown utm_campaign_name_3') as utm_campaign_name_3,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_4), 'Unknown utm_campaign_name_4') as utm_campaign_name_4,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_5), 'Unknown utm_campaign_name_5') as utm_campaign_name_5,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_6), 'Unknown utm_campaign_name_6') as utm_campaign_name_6,
            coalesce(String::ToLower(marketing_attribution_utm_campaign_name_7), 'Unknown utm_campaign_name_7') as utm_campaign_name_7,
            coalesce(String::ToLower(marketing_attribution_utm_medium), 'Unknown utm_medium') as utm_medium,
            coalesce(marketing_attribution_utm_term, 'Unknown utm_term') as utm_term
        FROM 
            `//home/cloud_analytics/marketing/attribution/attribution_flat`
    );
    
    $consumed_last_month = (
        SELECT DISTINCT
            billing_account_id, 1 as consumed_last_month
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 1=1
            AND event = 'day_use'
            AND real_consumption_vat > 0
            AND sku_lazy > 0
            AND $format_date($parse_datetime(event_time)) >= $format_date(DateTime::ShiftMonths(currentUtcDateTime(),-1))
    );
    
    $ba_segment = (
        SELECT DISTINCT
            billing_account_id, 
            MAX_BY(segment, event_time) as segment,
            MAX_BY(ba_person_type, event_time) as ba_person_type,
            String::JoinFromList(AGGREGATE_LIST_DISTINCT(service_name), ',') as service_name,
            String::JoinFromList(AGGREGATE_LIST_DISTINCT(subservice_name), ',') as subservice_name,
            DateTime::ToDays(DateTime::MakeTimestamp($parse_datetime(max(event_time))) - DateTime::MakeTimestamp($parse_datetime(min(event_time)))) as lifetime,
            sum(real_consumption_vat) as cons_all,
            sum(real_consumption_vat) / (0.00001 + DateTime::ToDays(DateTime::MakeTimestamp($parse_datetime(max(event_time))) - DateTime::MakeTimestamp($parse_datetime(min(event_time))))) * greatest(365, 0.00001 + DateTime::ToDays(DateTime::MakeTimestamp($parse_datetime(max(event_time))) - DateTime::MakeTimestamp($parse_datetime(min(event_time))))) as cons_1_year
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 1=1
            AND event = 'day_use'
        GROUP BY
            billing_account_id
    );
    
    $campaign_cost = (
        SELECT
            `campaign`,
            `medium`,
            `source`,
            sum(cost) as campaign_cost
        FROM `//home/marketing-data/money_log/datasets/project_2/project_2`
        GROUP BY
            `campaign`,
            `medium`,
            `source`
    );
    
    SELECT 
        events.billing_account_id as billing_account_id,
        ba_cohort_m,
        ba_cohort_w,
        ba_created_datetime,
        segment,
        ba_person_type,
        service_name,
        subservice_name,
        ba_count,
        first_trial_cons_count,
        first_paid_cons_count,
        consumed_last_month,
        cons_all,
        cons_1_year,
        if(ba_channel.billing_account_id = '', 'Unmatched', channel) as channel,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_source) as utm_source,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign) as utm_campaign,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name) as utm_campaign_name,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_1) as utm_campaign_name_1,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_2) as utm_campaign_name_2,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_3) as utm_campaign_name_3,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_4) as utm_campaign_name_4,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_5) as utm_campaign_name_5,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_6) as utm_campaign_name_6,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_campaign_name_7) as utm_campaign_name_7,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_medium) as utm_medium,
        if(ba_channel.billing_account_id = '', 'Unmatched', utm_term) as utm_term,
        -- if(ba_channel.billing_account_id = '', 1, event_weight) as event_weight,
        if(ba_channel.billing_account_id = '', 1, marketing_attribution_event_exp_7d_half_life_time_decay_weight) as marketing_attribution_event_exp_7d_half_life_time_decay_weight,
        if(ba_channel.billing_account_id = '', 1, marketing_attribution_event_first_weight) as marketing_attribution_event_first_weight,
        if(ba_channel.billing_account_id = '', 1, marketing_attribution_event_last_weight) as marketing_attribution_event_last_weight,
        if(ba_channel.billing_account_id = '', 1, marketing_attribution_event_uniform_weight) as marketing_attribution_event_uniform_weight,
        if(ba_channel.billing_account_id = '', 1, marketing_attribution_event_u_shape_weight) as marketing_attribution_event_u_shape_weight,
        campaign_cost
    FROM (
        SELECT 
            billing_account_id,
            sum(if(event_type = 'ba_created',1,0)) as ba_count,
            sum(if(event_type = 'first_trial_cons',1,0)) as first_trial_cons_count,
            sum(if(event_type = 'first_paid_cons',1,0)) as first_paid_cons_count
        FROM `//home/cloud_analytics/marketing/events_for_attribution`
        WHERE event_type in ('ba_created', 'first_trial_cons', 'first_paid_cons')
        GROUP BY 
            billing_account_id
    ) as events
        
    LEFT JOIN $ba_cohort_m as ba_cohort_m
    ON events.billing_account_id = ba_cohort_m.billing_account_id
    
    LEFT JOIN $ba_channel as ba_channel
    ON events.billing_account_id = ba_channel.billing_account_id
    LEFT JOIN $consumed_last_month as consumed_last_month
    ON events.billing_account_id = consumed_last_month.billing_account_id
    LEFT JOIN $ba_segment as ba_segment
    ON events.billing_account_id = ba_segment.billing_account_id
    LEFT JOIN $campaign_cost as campaign_cost
    ON (
        String::ToLower(ba_channel.utm_campaign) = String::ToLower(campaign_cost.campaign) 
    AND String::ToLower(ba_channel.utm_medium) = String::ToLower(campaign_cost.medium)
    AND String::ToLower(ba_channel.utm_source) = String::ToLower(campaign_cost.source)
    );
END DEFINE;

DEFINE action $funnel_uniform_raw($path) AS
    INSERT INTO $path WITH TRUNCATE
    SELECT * 
    FROM $result();
END DEFINE;


DEFINE action $funnel_uniform_final($path) AS
    INSERT INTO $path WITH TRUNCATE
    SELECT 
        billing_account_id,
        ba_cohort_m,
        ba_cohort_w,
        ba_created_datetime,
        segment,
        ba_person_type,
        service_name,
        subservice_name,
        ba_count,
        first_trial_cons_count,
        first_paid_cons_count,
        consumed_last_month,
        cons_all,
        cons_1_year,
        channel,
        `result`.utm_source as utm_source,
        `result`.utm_campaign as utm_campaign,
        `result`.utm_medium as utm_medium,
        utm_campaign_name,
        utm_campaign_name_1,
        utm_campaign_name_2,
        utm_campaign_name_3,
        utm_campaign_name_4,
        utm_campaign_name_5,
        utm_campaign_name_6,
        utm_campaign_name_7,
        utm_term,
        campaign_cost,
        marketing_attribution_event_exp_7d_half_life_time_decay_weight,
        marketing_attribution_event_exp_7d_half_life_time_decay_weight/marketing_attribution_event_exp_7d_half_life_time_decay_weight_sum as campaign_exp_7d_half_life_time_decay_weight,
        marketing_attribution_event_first_weight,
        marketing_attribution_event_first_weight/marketing_attribution_event_first_weight_sum as campaign_first_weight,
        marketing_attribution_event_last_weight,
        marketing_attribution_event_last_weight/marketing_attribution_event_last_weight_sum as campaign_last_weight,
        marketing_attribution_event_uniform_weight,
        marketing_attribution_event_uniform_weight/marketing_attribution_event_uniform_weight_sum as campaign_uniform_weight,
        marketing_attribution_event_u_shape_weight,
        marketing_attribution_event_u_shape_weight/marketing_attribution_event_u_shape_weight_sum as campaign_u_shape_weight
    FROM $result() as `result`
    LEFT JOIN (
        SELECT
            utm_campaign,
            utm_source,
            utm_medium,
            sum(marketing_attribution_event_exp_7d_half_life_time_decay_weight) as marketing_attribution_event_exp_7d_half_life_time_decay_weight_sum,
            sum(marketing_attribution_event_first_weight) as marketing_attribution_event_first_weight_sum,
            sum(marketing_attribution_event_last_weight) as marketing_attribution_event_last_weight_sum,
            sum(marketing_attribution_event_uniform_weight) as marketing_attribution_event_uniform_weight_sum,
            sum(marketing_attribution_event_u_shape_weight) as marketing_attribution_event_u_shape_weight_sum,
        FROM $result()
        GROUP BY 
            utm_campaign,
            utm_source,
            utm_medium
    ) as campaign_sum_weight
    ON (
        String::ToLower(`result`.utm_campaign) = String::ToLower(campaign_sum_weight.utm_campaign) 
    AND String::ToLower(`result`.utm_medium) = String::ToLower(campaign_sum_weight.utm_medium)
    AND String::ToLower(`result`.utm_source) = String::ToLower(campaign_sum_weight.utm_source)
    )
END DEFINE;

EXPORT $funnel_uniform_raw, $funnel_uniform_final;

