use hahn;

$date = ($str) -> {RETURN CAST(SUBSTRING($str, 0, 10) AS DATE)};
$result_table_path = '//home/cloud_analytics/dashboards/partner_cdm';

DEFINE ACTION $partner_cdm_script() AS

    $master_account_names = (
        --sql
        SELECT DISTINCT
                billing_account_id                                    as billing_account_id,
                IF(
                    crm_account_name != 'UNKNOWN', 
                    crm_account_name, 
                    billing_account_name)                             as master_account_name,
                partner_manager_current                               as partner_manager,
                crm_account_dimensions                                as partner_dimension
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags`
        WHERE
            usage_status_current != 'service' 
            AND is_var = true
        --endsql
    );

    $ba_first_paid_consumption = (
        --sql
        SELECT DISTINCT
            billing_account_id as ba_id,
            msk_event_dt as first_paid_consumption_date
        FROM `//home/cloud-dwh/data/prod/cdm/dm_events`
        WHERE
            event_type = 'billing_account_first_paid_consumption'
        --endsql
    );

    $consumption_raw = (
        --sql
        SELECT
            $date(billing_record_msk_date)                  as event_time,
            if((billing_master_account_id is null) or (billing_master_account_id = '') or (billing_master_account_id = 'UNKNOWN'), 
                billing_account_id,
                billing_master_account_id)                  as master_account_id,
            nvl(billing_account_id, '')
                in (select nvl(billing_account_id, '')
                    from $master_account_names)             as is_partner,
            billing_account_id,
            if(
                crm_account_name != 'UNKNOWN', 
                crm_account_name, 
                billing_account_name)                       as account_name,
            sku_service_name                                as service_name,
            sku_service_group                               as service_group,
            crm_segment_current                             as segment,

            billing_record_real_consumption_rub             as paid,
            billing_record_var_reward_rub                   as var_reward,
            billing_record_credit_monetary_grant_rub        as credit_grant,
            billing_record_credit_service_rub               as credit_service,
            billing_record_credit_cud_rub                   as credit_cud,
            billing_record_credit_volume_incentive_rub      as credit_volume_incentive,
            billing_record_credit_disabled_rub              as credit_disabled,
            billing_record_credit_trial_rub                 as credit_trial,

            billing_record_real_consumption_rub_vat         as paid_vat,
            billing_record_var_reward_rub_vat               as var_reward_vat,
            billing_record_credit_monetary_grant_rub_vat    as credit_grant_vat,
            billing_record_credit_service_rub_vat           as credit_service_vat,
            billing_record_credit_cud_rub_vat               as credit_cud_vat,
            billing_record_credit_volume_incentive_rub_vat  as credit_volume_incentive_vat,
            billing_record_credit_disabled_rub_vat          as credit_disabled_vat,
            billing_record_credit_trial_rub_vat             as credit_trial_vat,

            $date(first_paid_consumption_date) as first_paid_consumption_datetime
        FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`   as consumption
        LEFT JOIN $ba_first_paid_consumption                      as ba_first_paid_consumption
            ON consumption.billing_account_id = ba_first_paid_consumption.ba_id
        WHERE 
            (billing_account_is_var = true OR billing_account_is_subaccount = true)
            AND billing_account_usage_status_current != 'service'
        --endsql
    );

    $consumption = (
        SELECT
            event_time,
            master_account_id,
            is_partner,
            billing_account_id,
            account_name,
            service_name,
            service_group,
            segment,
            sum(paid)                            as paid,
            sum(var_reward)                      as var_reward,
            sum(credit_grant)                    as credit_grant,
            sum(credit_service)                  as credit_service,
            sum(credit_cud)                      as credit_cud,
            sum(credit_volume_incentive)         as credit_volume_incentive,
            sum(credit_disabled)                 as credit_disabled,
            sum(credit_trial)                    as credit_trial,
            sum(paid_vat)                        as paid_vat,
            sum(var_reward_vat)                  as var_reward_vat,
            sum(credit_grant_vat)                as credit_grant_vat,
            sum(credit_service_vat)              as credit_service_vat,
            sum(credit_cud_vat)                  as credit_cud_vat,
            sum(credit_volume_incentive_vat)     as credit_volume_incentive_vat,
            sum(credit_disabled_vat)             as credit_disabled_vat,
            sum(credit_trial_vat)                as credit_trial_vat,
            min(first_paid_consumption_datetime) as first_paid_consumption_datetime
        FROM $consumption_raw
        GROUP BY 
            event_time,
            master_account_id,
            is_partner,
            billing_account_id,
            account_name,
            service_name,
            service_group,
            segment
        --endsql
    );

    $partners_converted_date = (
        --sql
        SELECT 
            billing_account_id         as master_account_id, 
            MIN($date(date_qualified)) as converted_date
        FROM `//home/cloud_analytics/lunin-dv/dashboard_tables/var_info`
        WHERE 
            LENGTH(date_qualified) >= 10 
        GROUP BY 
            billing_account_id
        --endsql
    );

    $consumption_master_account = (
        --sql
        SELECT  
            master_account_names.billing_account_id                 as master_account_id,
            master_account_names.master_account_name                as master_account_name,
            master_account_names.partner_manager                    as partner_manager,
            master_account_names.partner_dimension                  as partner_dimension,
            partners_converted_date.converted_date                  as converted_date,
            if(consumption.is_partner, 1, 0)                        as is_partner,
            nvl(event_time, CurrentUtcDate())                       as event_time,
            nvl(consumption.billing_account_id, 'no_subaccount')    as billing_account_id,
            nvl(account_name, 'no_subaccount')                      as account_name,
            nvl(service_name, 'not_applicable')                     as service_name,
            nvl(service_group, 'not_applicable')                    as service_group,
            nvl(segment, 'no_segment')                              as segment,

            nvl(paid, 0)                                            as paid,           
            nvl(var_reward, 0)                                      as var_reward,     
            nvl(credit_grant, 0)                                    as credit_grant,       
            nvl(credit_service, 0)                                  as credit_service,         
            nvl(credit_cud, 0)                                      as credit_cud,     
            nvl(credit_volume_incentive, 0)                         as credit_volume_incentive,                  
            nvl(credit_disabled, 0)                                 as credit_disabled,          
            nvl(credit_trial, 0)                                    as credit_trial,  

            nvl(paid_vat, 0)                                        as paid_vat,
            nvl(var_reward_vat, 0)                                  as var_reward_vat,
            nvl(credit_grant_vat, 0)                                as credit_grant_vat,
            nvl(credit_service_vat, 0)                              as credit_service_vat,
            nvl(credit_cud_vat, 0)                                  as credit_cud_vat,
            nvl(credit_volume_incentive_vat, 0)                     as credit_volume_incentive_vat,
            nvl(credit_disabled_vat, 0)                             as credit_disabled_vat,
            nvl(credit_trial_vat, 0)                                as credit_trial_vat,

            first_paid_consumption_datetime
        FROM $master_account_names          as master_account_names
        LEFT JOIN $consumption              as consumption
            ON consumption.master_account_id = master_account_names.billing_account_id
        LEFT JOIN $partners_converted_date  as partners_converted_date
            ON partners_converted_date.master_account_id = master_account_names.billing_account_id
        --endsql
    );

    INSERT INTO $result_table_path WITH TRUNCATE
        SELECT *
        FROM $consumption_master_account

END DEFINE;

EXPORT $partner_cdm_script;
