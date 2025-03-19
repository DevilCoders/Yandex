use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date,  $format_date, $dates_range,$round_period_datetime,$round_period_date;

    

DEFINE SUBQUERY $lib_ba_count($params) AS

    DEFINE SUBQUERY $cube_raw_weekly($params) AS 
        SELECT DISTINCT 
            $round_period_date($params['period'],billing_record_msk_date) as `date`,
            billing_record_msk_date,
            billing_account_id,
            sku_service_name, 
            sku_subservice_name,
            crm_segment,
            billing_record_real_consumption_rub_vat
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
        WHERE 
            sku_lazy = 0
    END DEFINE;

    DEFINE SUBQUERY $ba_support_type($params) AS 
        SELECT  
            `date`,
            billing_account_id,
            max_by(sku_subservice_name, billing_record_msk_date) as ba_support_type
        FROM $cube_raw_weekly($params)
        WHERE 
            sku_service_name = 'support'
        GROUP BY 
            `date`,
            billing_account_id

    END DEFINE;


    SELECT 
        `date`,
        count(DISTINCT billing_account_id) as ba_count
    FROM (
        SELECT 
            ba_tags.`date` as `date`,
            ba_tags.billing_account_id as billing_account_id,
            ba_paid_status,
            crm_segment,
            coalesce(ba_support_type,'free') as ba_support_type
        FROM (
            SELECT 
                `date`,
                billing_account_id,
                if(sum(billing_record_real_consumption_rub_vat)>0, 'paid', 'trial') as ba_paid_status,
                max_by(crm_segment,billing_record_msk_date) as crm_segment
            FROM $cube_raw_weekly($params)
            GROUP BY 
                `date`,
                billing_account_id
        ) as ba_tags 
        LEFT JOIN 
            $ba_support_type($params) as ba_support_type
        ON 
            ba_tags.`date` = ba_support_type.`date`
            AND ba_tags.billing_account_id =  ba_support_type.billing_account_id
    ) 
    WHERE   
        CASE
            WHEN $params['mode'] = 'support_tickets_count_per_ba_all' then True
            WHEN $params['mode'] = 'support_tickets_count_per_ba_ba_paid_status_paid' then ba_paid_status = 'paid'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_ba_paid_status_trial' then ba_paid_status = 'trial'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_segment_mass' then crm_segment = 'Mass'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_segment_medium' then crm_segment = 'Medium'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_segment_enterprise' then crm_segment = 'Enterprise'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_support_type_free' then ba_support_type = 'free'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_support_type_allpaid' then ba_support_type in ('standard','business','premium')
            WHEN $params['mode'] = 'support_tickets_count_per_ba_support_type_standard' then ba_support_type = 'standard'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_support_type_business' then ba_support_type = 'business'
            WHEN $params['mode'] = 'support_tickets_count_per_ba_support_type_premium' then ba_support_type = 'premium'
            ELSE True
        END
    GROUP BY 
        `date`
    

END DEFINE;



EXPORT $lib_ba_count;