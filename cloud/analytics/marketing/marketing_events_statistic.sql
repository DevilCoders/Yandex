use hahn;

-- PRAGMA Library('tables.sql');
-- PRAGMA Library('time.sql');
-- PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA yson.DisableStrict ;

IMPORT tables SYMBOLS $last_non_empty_table;
IMPORT time SYMBOLS $format_date;

$get_group = ($uid) -> {
    $hash = Digest::Crc64($uid);
    $group_number = $hash % 100;
    RETURN $group_number
};

DEFINE subquery $puid_ba_cloud() AS (
SELECT 
    puid,
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(cloud_id), ',') as cloud_id, 
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(billing_account_id), ',') as billing_account_id,
    MIN(cloud_cohort_w) as cloud_cohort_w,
    MIN(cloud_cohort_m) as cloud_cohort_m
FROM `//home/cloud_analytics/marketing/ba_cloud_puid`
--FROM `//home/cloud_analytics/ba_tags/ba_cloud_puid` --test for marketing_change
GROUP BY 
    puid
);
END DEFINE;

DEFINE subquery $events_services() AS (
SELECT 
    events_services.event_id as event_id, 
    String::JoinFromList(AGGREGATE_LIST(name), ',') as services
FROM $last_non_empty_table('//home/cloud_analytics/dwh/raw/backoffice/events_services') as events_services
LEFT JOIN $last_non_empty_table('//home/cloud_analytics/dwh/raw/backoffice/services') as services
ON events_services.service_id = services.id
GROUP BY events_services.event_id
);
END DEFINE;

DEFINE subquery $events_visits() AS (
--добавить Scale
SELECT 
    event_id,
    sum(Sign) as visits_count
FROM (
SELECT 
    IF( 
        StartURL like '%https://cloud.yandex.ru/events/scale-2020%', '149',
        String::SplitToList(String::SplitToList(StartURL,'/')[4],'?')[0]
    ) as event_id,
    Sign
FROM RANGE('//home/cloud_analytics/import/metrika')
WHERE 
    (CounterID = 51465824 AND StartURL like '%https://cloud.yandex.ru/events/%')
    OR
    (CounterID = 50027884 AND StartURL like '%https://cloud.yandex.ru/events/scale-2020%')
)
GROUP BY event_id

);
END DEFINE;

$extract_participant_position_lower = ($x) -> { 
    RETURN String::ToLower(Yson::ConvertToString(CAST($x AS Json).position))
};

$determine_participant_position_type = ($position) -> {
    RETURN CASE
        WHEN 
            $position like '%маркет%' or 
            $position like '%market%' or 
            $position = 'pr'
        THEN 'Marketing'
        WHEN 
            $position like '%разработ%' or 
            $position like '%програм%' or
            $position like '%devel%' or
            $position like '%инж%' or
            $position like '%engineer%'
        THEN 'Software Developer'
        WHEN
            $position like '%анал%' or 
            $position like '%anal%' or
            $position like '%data%'
        THEN 'Analyst/Data Scientist'
        WHEN 
            $position like '%адм%' or 
            $position like '%ops%' or
            $position like '%пп%'
        THEN 'SysAdmin/DevOps'
        WHEN 
            $position like '%продукт%' or 
            $position like '%продакт%' or 
            $position like '%product%' or
            $position like '%проект%' or 
            $position = 'pm' or 
            $position like '%project%'
        THEN 'Product/Project Manager'
        WHEN 
            $position like '%руковод%' or
            $position like '%head%' or
            $position like '%нач%' or
            $position like '%менедж%' or
            $position like '%lead%'
        THEN 'Middle Manager'
        WHEN 
            $position like '%директор%' or
            $position like '%дир%'
        THEN 'Director'
        WHEN 
            $position like '%студент%'
        THEN 'Student'
        WHEN 
            $position like '%архитект%' or
            $position like '%archit%'
        THEN 'Architect'
        WHEN 
            $position = 'ип'
        THEN 'Private Entrepreneur'
        WHEN 
            $position like '%продаж%' or
            $position like '%sales%' or
            $position like '%сейл%' or
            $position like '%account%'
        THEN 'Sales Manager'
        WHEN 
            $position = 'ceo' or
            $position = 'cto' or
            $position = 'cpo' or
            $position = 'cio' or
            $position = 'ceo' or
            $position = 'ceo' 
        THEN 'C*O'
        ELSE 'Other'
        END

};


DEFINE subquery $events_stats() AS (
SELECT
    applications.id as application_id,
    applications.created_at as application_datetime,
    $format_date(CurrentUtcTimestamp()) as experiment_date,
    status as application_status,
    CAST(applications.event_id AS String) as event_id,
    events.`date` as event_time,
    $format_date(Datetime::FromSeconds(CAST(events.`date` AS Uint32))) as event_date,
    events.name_ru as event_name,
    events.description_ru as event_description,
    events.short_description_ru as event_description_short,
    events.record as event_record,
    events.registration_form_hashed_id as event_registration_form_id,
    coalesce(events.url, 'https://cloud.yandex.ru/events/' || CAST (applications.event_id AS String)) as event_url,
    events.webinar_url as event_registration_url,
    events.registration_status as event_registration_status,
    events.is_online as event_is_online,
    participant_id,
    Yson::ConvertToString(CAST(flat_data AS Json).name) as participant_name,
    Yson::ConvertToString(CAST(flat_data AS Json).last_name) as participant_last_name,
    Yson::ConvertToString(CAST(flat_data AS Json).work) as participant_company_name,
    Yson::ConvertToString(CAST(flat_data AS Json).industry) as participant_company_industry,
    Yson::ConvertToString(CAST(flat_data AS Json).answer_choices_10334855) as participant_company_size,
    Yson::ConvertToString(CAST(flat_data AS Json).position) as participant_position,
    $determine_participant_position_type($extract_participant_position_lower(flat_data)) as participant_position_type,
    Yson::ConvertToString(CAST(flat_data AS Json).phone) as participant_phone,
    Yson::ConvertToString(CAST(flat_data AS Json).email) as participant_email,
    Yson::ConvertToString(CAST(flat_data AS Json).website) as participant_website,
    Yson::ConvertToString(CAST(flat_data AS Json).user_status) as participant_has_yacloud_account,
    Yson::ConvertToString(CAST(flat_data AS Json).answer_choices_10222008) as participant_scenario,
    Yson::ConvertToString(CAST(flat_data AS Json).answer_choices_10222012) as participant_need_manager_call,
    Yson::ConvertToString(CAST(flat_data AS Json).answer_choices_10231648) as participant_infrastructure_type,
    Yson::ConvertToString(CAST(flat_data AS Json).answer_choices_10222014) as participant_how_did_know_about_event,
    COALESCE(
        Yson::ConvertToString(CAST(flat_data AS Json).answer_boolean_10222015),
        COALESCE(
            Yson::ConvertToString(CAST(flat_data AS Json).answer_boolean_10166162),
            Yson::ConvertToString(CAST(flat_data AS Json).agreementNewsletters) 
        )
    ) as participant_agree_to_communicate,
    uid as participant_puid,
    cloud_id as participant_cloud_id,
    billing_account_id as participant_billing_account_id,
    events_costs.event_ticket as event_ticket,
    events_costs.event_budget_wo_vat as event_budget_wo_vat, 
    services,
    visits_count
FROM $last_non_empty_table('//home/cloud_analytics/dwh/raw/backoffice/applications') as applications
LEFT JOIN $last_non_empty_table('//home/cloud_analytics/dwh/raw/backoffice/participants') as participants
ON applications.participant_id = participants.id
LEFT JOIN $puid_ba_cloud() as puid_ba_cloud
ON CAST(participants.uid AS String) = puid_ba_cloud.puid
LEFT JOIN $last_non_empty_table('//home/cloud_analytics/dwh/raw/backoffice/events') as events
ON applications.event_id = events.id
LEFT JOIN (
SELECT 
    backoffice_event_id,
    event_ticket,
    sum(event_budget_wo_vat) as event_budget_wo_vat
FROM
    `//home/cloud_analytics/marketing/events/events_costs` 
GROUP BY 
    backoffice_event_id,
    event_ticket
) as events_costs
ON CAST(applications.event_id AS String) = events_costs.backoffice_event_id
LEFT JOIN $events_services() as events_services
ON applications.event_id = events_services.event_id
LEFT JOIN $events_visits() as events_visits
ON CAST(applications.event_id AS String) = events_visits.event_id
);
END DEFINE;

DEFINE subquery $application_cloud_flat() AS (
    SELECT 
        application_id,
        cloud_id
    FROM (
        SELECT
            application_id,
            String::SplitToList(participant_cloud_id,',') as cloud_id
        FROM 
            $events_stats()
        )
    FLATTEN BY cloud_id
);
END DEFINE;

DEFINE subquery $application_first_cloud_created() AS (
    SELECT 
        application_cloud.application_id as application_id,
        MIN_BY(application_cloud.cloud_id, event_date) as first_cloud_id,
        min(event_date) as first_cloud_created_date
    FROM $application_cloud_flat() as application_cloud
    LEFT JOIN (
        SELECT 
            cloud_id,
            event_date
        FROM `//home/cloud_analytics/marketing/events_for_attribution` 
        WHERE event_type = 'cloud_created'
    ) as events
    ON application_cloud.cloud_id = events.cloud_id
    GROUP BY 
        application_cloud.application_id
);
END DEFINE;

DEFINE subquery $application_ba_flat() AS (
    SELECT 
        application_id,
        billing_account_id
    FROM (
        SELECT
            application_id,
            String::SplitToList(participant_billing_account_id,',') as billing_account_id
        FROM 
            $events_stats()
        )
    FLATTEN BY billing_account_id
);
END DEFINE;

DEFINE subquery $application_first_ba_created() AS (
    SELECT 
        application_ba.application_id as application_id,
        min(event_date) as first_ba_created_date
    FROM $application_ba_flat() as application_ba
    LEFT JOIN (
        SELECT 
            billing_account_id,
            event_date
        FROM `//home/cloud_analytics/marketing/events_for_attribution` 
        WHERE event_type = 'ba_created'
    ) as events
    ON application_ba.billing_account_id = events.billing_account_id
    GROUP BY 
        application_ba.application_id
);
END DEFINE;

DEFINE subquery $application_first_trial_cons() AS (
    SELECT 
        application_ba.application_id as application_id,
        min(event_date) as first_trial_cons_date
    FROM $application_ba_flat() as application_ba
    LEFT JOIN (
        SELECT 
            billing_account_id,
            event_date
        FROM `//home/cloud_analytics/marketing/events_for_attribution` 
        WHERE event_type = 'first_trial_cons'
    ) as events
    ON application_ba.billing_account_id = events.billing_account_id
    GROUP BY 
        application_ba.application_id
);
END DEFINE;

DEFINE subquery $application_first_paid_cons() AS (
    SELECT 
        application_ba.application_id as application_id,
        min(event_date) as first_paid_cons_date
    FROM $application_ba_flat() as application_ba
    LEFT JOIN (
        SELECT 
            billing_account_id,
            event_date
        FROM `//home/cloud_analytics/marketing/events_for_attribution` 
        WHERE event_type = 'first_paid_cons'
    ) as events
    ON application_ba.billing_account_id = events.billing_account_id
    GROUP BY 
        application_ba.application_id
);
END DEFINE;

DEFINE subquery $marketing_event_attribution_weights() AS (
    SELECT
    attr.billing_account_id as billing_account_id,
    attr.marketing_attribution_event_exp_7d_half_life_time_decay_weight as marketing_attribution_event_exp_7d_half_life_time_decay_weight,
    attr.marketing_attribution_event_id as marketing_attribution_event_id,
    events.marketing_event_application_id as marketing_event_application_id
    FROM (SELECT * FROM `//home/cloud_analytics/marketing/attribution/attribution_flat`) as attr
    LEFT JOIN (SELECT * FROM `//home/cloud_analytics/marketing/events_for_attribution`) as events
    ON attr.marketing_attribution_event_id = events.event_id
    WHERE marketing_event_application_id is not null
);
END DEFINE;

DEFINE subquery $ba_consumption() AS (
    SELECT 
        billing_account_id,
        sum(real_consumption_vat) as paid
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    GROUP BY billing_account_id
);
END DEFINE;

DEFINE SUBQUERY $ba_managers() AS 
    SELECT 
        billing_account_id,
        AGGREGATE_LIST_DISTINCT(segment_current)[0] as crm_account_segment_current,
        AGGREGATE_LIST_DISTINCT(account_owner_current)[0] as crm_account_owner_current,
        AGGREGATE_LIST_DISTINCT(architect_current)[0] as crm_account_architect_current,
        AGGREGATE_LIST_DISTINCT(bus_dev_current)[0] as crm_account_bus_dev_current,
        AGGREGATE_LIST_DISTINCT(partner_manager_current)[0] as crm_account_partner_manager_current,
        AGGREGATE_LIST_DISTINCT(sales_current)[0] as crm_account_sales_current
    FROM 
        `//home/cloud_analytics/ba_tags/ba_crm_tags`
    GROUP BY    
        billing_account_id
END DEFINE;

DEFINE subquery $final_result() AS (
    SELECT
        events_stats.application_id as application_id,
        events_stats.application_datetime as application_datetime,
        experiment_date,
        application_status,
        event_id,
        event_time,
        event_date, 
        event_name,
        event_description,
        event_description_short,
        event_record,
        event_registration_form_id,
        event_url,
        event_registration_url,
        event_registration_status,
        event_is_online,
        participant_id,
        participant_name,
        participant_last_name,
        participant_company_name,
        participant_company_industry,
        participant_company_size,
        participant_position,
        participant_position_type,
        participant_phone,
        participant_email,
        participant_website,
        participant_has_yacloud_account,
        participant_scenario,
        participant_need_manager_call,
        participant_infrastructure_type,
        participant_how_did_know_about_event,
        participant_agree_to_communicate,
        participant_puid,
        participant_cloud_id,
        participant_billing_account_id,
        event_ticket,
        event_budget_wo_vat, 
        services as event_services,
        visits_count as event_visits_count,
        first_cloud_created_date as participant_first_cloud_created_date,
        first_cloud_id as participant_first_cloud_id,
        first_ba_created_date as participant_first_ba_created_date,
        first_trial_cons_date as participant_first_trial_cons_date,
        first_paid_cons_date as participant_first_paid_cons_date, 
        cloud_owners_history.cloud_name as participant_first_cloud_name,
        cloud_owners_history.cloud_status as participant_first_cloud_status,
        cloud_owners_history.email as participant_first_cloud_email,
        cloud_owners_history.first_name as participant_first_cloud_first_name,
        cloud_owners_history.last_name as participant_first_cloud_last_name,
        cloud_owners_history.login as participant_first_cloud_login,
        cloud_owners_history.mail_billing as participant_first_cloud_mail_billing,
        cloud_owners_history.mail_event as participant_first_cloud_mail_event,
        cloud_owners_history.mail_feature as participant_first_cloud_mail_feature,
        cloud_owners_history.mail_info as participant_first_cloud_mail_info,
        cloud_owners_history.mail_marketing as participant_first_cloud_mail_marketing,
        cloud_owners_history.mail_promo as participant_first_cloud_mail_promo,
        cloud_owners_history.mail_support as participant_first_cloud_mail_support,
        cloud_owners_history.mail_tech as participant_first_cloud_mail_tech,
        cloud_owners_history.mail_technical as participant_first_cloud_mail_technical,
        cloud_owners_history.mail_testing as participant_first_cloud_mail_testing,
        cloud_owners_history.passport_uid as participant_first_cloud_passport_uid,
        cloud_owners_history.phone as participant_first_cloud_phone,
        cloud_owners_history.timezone as participant_first_cloud_timezone,
        cloud_owners_history.user_settings_email as participant_first_cloud_user_settings_email,
        cloud_owners_history.user_settings_language as participant_first_cloud_user_settings_language, 
        COALESCE(cloud_owners_history.user_settings_email, participant_email) as email,
        $get_group(COALESCE(cloud_owners_history.user_settings_email, participant_email)) as email_group,
        IF($get_group(COALESCE(cloud_owners_history.user_settings_email, participant_email)) >=35 
            and $get_group(COALESCE(cloud_owners_history.user_settings_email, participant_email)) <=44, 
            'control',  
            'test'
        ) as `group`,
        ba_weight.marketing_attribution_event_exp_7d_half_life_time_decay_weight as marketing_attribution_event_exp_7d_half_life_time_decay_weight,
        ba_cons.paid as paid,
        ba_managers.crm_account_segment_current AS participant_crm_account_segment_current,
        ba_managers.crm_account_owner_current AS participant_crm_account_owner_current,
        ba_managers.crm_account_architect_current AS participant_crm_account_architect_current,
        ba_managers.crm_account_bus_dev_current AS participant_crm_account_bus_dev_current,
        ba_managers.crm_account_partner_manager_current AS participant_crm_account_partner_manager_current,
        ba_managers.crm_account_sales_current AS participant_crm_account_sales_current

    FROM $events_stats() as events_stats
    LEFT JOIN $application_first_cloud_created() as application_first_cloud_created
    ON events_stats.application_id = application_first_cloud_created.application_id
    LEFT JOIN $application_first_ba_created() as application_first_ba_created
    ON events_stats.application_id = application_first_ba_created.application_id
    LEFT JOIN $application_first_trial_cons() as application_first_trial_cons
    ON events_stats.application_id = application_first_trial_cons.application_id
    LEFT JOIN $application_first_paid_cons() as application_first_paid_cons
    ON events_stats.application_id = application_first_paid_cons.application_id
    LEFT JOIN `//home/cloud_analytics/import/iam/cloud_owners_history` as cloud_owners_history
    ON application_first_cloud_created.first_cloud_id = cloud_owners_history.cloud_id
    LEFT JOIN $marketing_event_attribution_weights() as ba_weight
    ON String::SplitToList(events_stats.participant_billing_account_id,',')[0] = ba_weight.billing_account_id
    AND events_stats.application_id = ba_weight.marketing_event_application_id 
    LEFT JOIN $ba_consumption() as ba_cons
    ON String::SplitToList(events_stats.participant_billing_account_id,',')[0] = ba_cons.billing_account_id 
    LEFT JOIN $ba_managers() as ba_managers
    ON String::SplitToList(events_stats.participant_billing_account_id,',')[0] = ba_managers.billing_account_id
);
END DEFINE;

--INSERT INTO `//home/cloud_analytics/marketing/events/events_stats` WITH TRUNCATE
DEFINE action $get_events_stats($path) AS 
    INSERT INTO $path WITH TRUNCATE
    SELECT * FROM 
    $final_result();
END DEFINE;

--INSERT INTO `//home/cloud_analytics/marketing/events/ba_events_tags` WITH TRUNCATE
DEFINE action $get_ba_events_tags($path) AS 
    INSERT INTO $path WITH TRUNCATE
    SELECT 
        billing_account_id,
        count(distinct event_id) as events_count,
        String::JoinFromList(AGGREGATE_LIST_DISTINCT(CAST(event_id AS String)), ',') as events_list
    FROM (
        SELECT 
            event_id,
            billing_account_id
        FROM (
            SELECT
                event_id,
                String::SplitToList(participant_billing_account_id,',') as billing_account_id
            FROM 
                $events_stats()
            )
        FLATTEN BY billing_account_id
    )
    GROUP BY
        billing_account_id;
END DEFINE;


EXPORT $get_ba_events_tags, $get_events_stats;
