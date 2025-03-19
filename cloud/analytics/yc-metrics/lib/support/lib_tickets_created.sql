use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date,  $format_date, $dates_range,$round_period_datetime,$round_period_date,$round_current_time,$round_period_format;

    

DEFINE SUBQUERY $lib_tickets_created($params) AS
    SELECT 
        tickets.`date` as `date`,
        count(DISTINCT tickets.startrek_key) as tickets_count
    FROM (
        SELECT
            $round_period_format($params['period'],issues.created_at) as `date`,
            issues.issue_key  as startrek_key,
            issues.billing_account_id AS billing_account_id,
            if(payment_tariff not in ('standard','premium','business'),'free',payment_tariff) as pay
        FROM
            `//home/cloud-dwh/data/prod/cdm/support/dm_yc_support_issues` as issues
        WHERE 
            components_quotas = False 
        GROUP BY 
            issues.created_at,
            issues.issue_key,
            issues.billing_account_id,
            payment_tariff
    ) as tickets 
    LEFT JOIN (
        SELECT
            billing_account_id,
            `date`,
            IF(sum(real_consumption_vat) > 0, 'paid', 'trial') as ba_paid_status,
            max_by(crm_segment, billing_record_msk_date) as crm_segment
        FROM (
            SELECT
                billing_account_id,
                $round_period_date($params['period'],billing_record_msk_date) as `date`,
                billing_record_msk_date,
                billing_record_real_consumption_rub_vat as real_consumption_vat,
                crm_segment
            FROM 
               `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
        )
        GROUP BY 
            `date`,
            billing_account_id
    ) as ba_paid_status 
    ON 
        tickets.`date` = ba_paid_status.`date`
        AND tickets.billing_account_id = ba_paid_status.billing_account_id
    WHERE
        CASE
            WHEN $params['mode'] = 'support_tickets_count_all' then True
            WHEN $params['mode'] = 'support_tickets_count_ba_paid_status_paid' then ba_paid_status = 'paid'
            WHEN $params['mode'] = 'support_tickets_count_ba_paid_status_trial' then ba_paid_status = 'trial'
            WHEN $params['mode'] = 'support_tickets_count_segment_mass' then crm_segment = 'Mass'
            WHEN $params['mode'] = 'support_tickets_count_segment_medium' then crm_segment = 'Medium'
            WHEN $params['mode'] = 'support_tickets_count_segment_enterprise' then crm_segment = 'Enterprise'
            WHEN $params['mode'] = 'support_tickets_count_support_type_free' then pay = 'free'
            WHEN $params['mode'] = 'support_tickets_count_support_type_allpaid' then pay in ('standard','business','premium')
            WHEN $params['mode'] = 'support_tickets_count_support_type_standard' then pay = 'standard'
            WHEN $params['mode'] = 'support_tickets_count_support_type_business' then pay = 'business'
            WHEN $params['mode'] = 'support_tickets_count_support_type_premium' then pay = 'premium'
            ELSE True
        END
    GROUP BY 
        tickets.`date`

END DEFINE;


EXPORT $lib_tickets_created;