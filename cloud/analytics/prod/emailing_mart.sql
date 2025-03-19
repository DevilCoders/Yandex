USE hahn;

DEFINE SUBQUERY $emailing_mart() AS 
    DEFINE SUBQUERY $passport_users() AS 
        SELECT 
            iam_uid,
            created_at as iam_created_at,
            created_clouds,
            email_hash,
            phone_hash,
            passport_uid,
            case 
                when language = 'kk' then language
                when language in ('ru','be','uz','az','tt','hy','ka') then 'ru'
                else 'en'
            end as language,
            mail_alerting,
            mail_billing,
            mail_event,
            mail_feature,
            mail_info,
            mail_promo,
            mail_tech,
            mail_testing
        FROM 
            `//home/cloud-dwh/data/prod/ods/iam/passport_users`
    END DEFINE;

    DEFINE SUBQUERY $federated_users() AS 
        SELECT 
            iam_user_id as iam_uid,
            created_at as iam_created_at,
            NULL as created_clouds,
            email_hash,
            phone_hash,
            NULL as passport_uid,
            NULL as language,
            mail_alerting,
            mail_billing,
            mail_event,
            mail_feature,
            mail_info,
            mail_promo,
            mail_tech,
            mail_testing
        FROM 
            `//home/cloud-dwh/data/prod/ods/iam/federated_users`
    END DEFINE;

    DEFINE SUBQUERY $iam_users() AS
        SELECT 
            *
        FROM    
            $passport_users()
        UNION ALL
        SELECT 
            *
        FROM    
            $federated_users()
    END DEFINE;

    DEFINE SUBQUERY $clouds() AS
        SELECT
            cloud_id,
            created_at as cloud_created_at,
            created_by_iam_uid,
            status as cloud_status
        FROM 
            `//home/cloud-dwh/data/prod/ods/iam/clouds`
    END DEFINE;

    DEFINE SUBQUERY $cloud_ba() AS 
        SELECT 
            service_instance_id as cloud_id,
            MAX_BY(billing_account_id, end_time) as ba_id
        FROM 
            `//home/cloud-dwh/data/prod/ods/billing/service_instance_bindings`
        WHERE   service_instance_type = 'cloud'
        GROUP BY
            service_instance_id
    END DEFINE;

    DEFINE SUBQUERY $billing_accounts() AS 
        SELECT  
            billing_account_id,
            created_at as billing_account_created_at,
            country_code,
            person_type,
            state,
            usage_status,
            is_isv,
            is_suspended_by_antifraud,
            owner_iam_uid,
            owner_passport_uid
        FROM 
            `//home/cloud-dwh/data/prod/ods/billing/billing_accounts`
    END DEFINE;

    DEFINE SUBQUERY $consumption() AS
        SELECT
            billing_account_id,
            min(if(billing_record_total_rub_vat != 0, billing_record_msk_date, Null)) as first_paid_cons_date,
            max(if(billing_record_total_rub_vat != 0, billing_record_msk_date, Null)) as last_paid_cons_date,
            min(if(billing_record_credit_monetary_grant_rub_vat != 0, billing_record_msk_date, Null)) as first_trial_cons_date,
            max(if(billing_record_credit_monetary_grant_rub_vat != 0, billing_record_msk_date, Null)) as last_trial_cons_date,
            Math::Round(sum(billing_record_total_rub_vat)) as total_paid_cons,
            Math::Round(sum(billing_record_credit_monetary_grant_rub_vat)*(-1)) as total_trial_cons
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
        GROUP BY
            billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $emailing_mart_raw() AS
        SELECT
            *
        FROM 
            $iam_users() as iam
        LEFT JOIN 
            $clouds() as clouds
        ON iam.iam_uid = clouds.created_by_iam_uid
        LEFT JOIN 
            $cloud_ba() as cloud_ba
        ON clouds.cloud_id = cloud_ba.cloud_id
        LEFT JOIN
            $billing_accounts() as bas
        ON iam.iam_uid = bas.owner_iam_uid
        AND cloud_ba.ba_id = bas.billing_account_id
        LEFT JOIN
            $consumption() as cons 
        ON bas.billing_account_id = cons.billing_account_id
    END DEFINE;

    SELECT 
        iam_uid                     as iam_uid,
        iam_created_at              as iam_created_at,
        email_hash                  as iam_email_hash,
        phone_hash                  as iam_phone_hash,
        passport_uid                as iam_puid,
        language                    as iam_language,
        mail_alerting               as iam_mail_alerting,
        mail_billing                as iam_mail_billing, 
        mail_event                  as iam_mail_event,
        mail_feature                as iam_mail_feature,
        mail_info                   as iam_mail_info,
        mail_promo                  as iam_mail_promo,
        mail_tech                   as iam_mail_tech,
        mail_testing                as iam_mail_testing,
        cloud_id                    as cloud_id,
        cloud_created_at            as cloud_created_at,
        cloud_status                as cloud_status,
        billing_account_id          as ba_id,
        billing_account_created_at  as ba_created_at,
        country_code                as ba_country_code,
        person_type                 as ba_person_type,
        state                       as ba_state,
        usage_status                as ba_usage_status,
        is_isv                      as ba_is_isv,
        is_suspended_by_antifraud   as ba_is_fraud,
        first_paid_cons_date        as ba_first_paid_cons,
        last_paid_cons_date         as ba_last_paid_cons,
        first_trial_cons_date       as ba_first_trial_cons,
        last_trial_cons_date        as ba_last_trial_cons,
        total_paid_cons             as ba_total_paid_cons,
        total_trial_cons            as ba_total_trial_cons
    FROM 
        $emailing_mart_raw()
    WHERE
        billing_account_id is not NULL
        OR DateTime::ToDays(DateTime::MakeDatetime(CurrentUtcDatetime()) - DateTime::MakeDatetime(iam_created_at)) < 60
END DEFINE;

EXPORT $emailing_mart;
