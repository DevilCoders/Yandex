use hahn;

--`//home/cloud_analytics/mas678/marketing/events/events_stats_temp`
DEFINE ACTION $events_stats_temp($path) AS
INSERT INTO $path WITH TRUNCATE 
SELECT 
    events_stats.application_datetime AS application_datetime,
    events_stats.application_id AS application_id,
    events_stats.application_status AS application_status,
    events_stats.email AS email,
    events_stats.email_group AS email_group,
    events_stats.event_budget_wo_vat AS event_budget_wo_vat,
    events_stats.event_date AS event_date,
    events_stats.event_description AS event_description,
    events_stats.event_description_short AS event_description_short,
    events_stats.event_id AS event_id,
    events_stats.event_is_online AS event_is_online,
    events_stats.event_name AS event_name,
    events_stats.event_record AS event_record,
    events_stats.event_registration_form_id AS event_registration_form_id,
    events_stats.event_registration_status AS event_registration_status,
    events_stats.event_registration_url AS event_registration_url,
    events_stats.event_services AS event_services,
    events_stats.event_ticket AS event_ticket,
    events_stats.event_url AS event_url,
    events_stats.event_visits_count AS event_visits_count,
    events_stats.expirement_date AS expirement_date,
    events_stats.participant_agree_to_communicate AS participant_agree_to_communicate,
    events_stats.participant_billing_account_id AS participant_billing_account_id,
    events_stats.participant_cloud_id AS participant_cloud_id,
    events_stats.participant_company_industry AS participant_company_industry,
    events_stats.participant_company_name AS participant_company_name,
    events_stats.participant_company_size AS participant_company_size,
    events_stats.participant_email AS participant_email,
    events_stats.participant_first_ba_created_date AS participant_first_ba_created_date,
    events_stats.participant_first_cloud_created_date AS participant_first_cloud_created_date,
    events_stats.participant_first_cloud_email AS participant_first_cloud_email,
    events_stats.participant_first_cloud_first_name AS participant_first_cloud_first_name,
    events_stats.participant_first_cloud_id AS participant_first_cloud_id,
    events_stats.participant_first_cloud_last_name AS participant_first_cloud_last_name,
    events_stats.participant_first_cloud_login AS participant_first_cloud_login,
    events_stats.participant_first_cloud_mail_billing AS participant_first_cloud_mail_billing,
    events_stats.participant_first_cloud_mail_event AS participant_first_cloud_mail_event,
    events_stats.participant_first_cloud_mail_feature AS participant_first_cloud_mail_feature,
    events_stats.participant_first_cloud_mail_info AS participant_first_cloud_mail_info,
    events_stats.participant_first_cloud_mail_marketing AS participant_first_cloud_mail_marketing,
    events_stats.participant_first_cloud_mail_promo AS participant_first_cloud_mail_promo,
    events_stats.participant_first_cloud_mail_support AS participant_first_cloud_mail_support,
    events_stats.participant_first_cloud_mail_tech AS participant_first_cloud_mail_tech,
    events_stats.participant_first_cloud_mail_technical AS participant_first_cloud_mail_technical,
    events_stats.participant_first_cloud_mail_testing AS participant_first_cloud_mail_testing,
    events_stats.participant_first_cloud_name AS participant_first_cloud_name,
    events_stats.participant_first_cloud_passport_uid AS participant_first_cloud_passport_uid,
    events_stats.participant_first_cloud_phone AS participant_first_cloud_phone,
    events_stats.participant_first_cloud_status AS participant_first_cloud_status,
    events_stats.participant_first_cloud_timezone AS participant_first_cloud_timezone,
    events_stats.participant_first_cloud_user_settings_email AS participant_first_cloud_user_settings_email,
    events_stats.participant_first_cloud_user_settings_language AS participant_first_cloud_user_settings_language,
    events_stats.participant_first_paid_cons_date AS participant_first_paid_cons_date,
    events_stats.participant_first_trial_cons_date AS participant_first_trial_cons_date,
    events_stats.participant_has_yacloud_account AS participant_has_yacloud_account,
    events_stats.participant_how_did_know_about_event AS participant_how_did_know_about_event,
    events_stats.participant_id AS participant_id,
    events_stats.participant_infrastructure_type AS participant_infrastructure_type,
    events_stats.participant_last_name AS participant_last_name,
    events_stats.participant_name AS participant_name,
    events_stats.participant_need_manager_call AS participant_need_manager_call,
    events_stats.participant_phone AS participant_phone,
    events_stats.participant_position AS participant_position,
    events_stats.participant_position_type AS participant_position_type,
    events_stats.participant_puid AS participant_puid,
    events_stats.participant_scenario AS participant_scenario,
    events_stats.participant_website AS participant_website,
    ba_weight.marketing_attribution_event_exp_7d_half_life_time_decay_weight as marketing_attribution_event_exp_7d_half_life_time_decay_weight,
    ba_cons.paid as paid
FROM (
    SELECT * FROM `//home/cloud_analytics/marketing/events/events_stats`
) as events_stats
LEFT JOIN (
    SELECT
    "billing_account_id" as billing_account_id,
    "marketing_attribution_event_exp_7d_half_life_time_decay_weight" as marketing_attribution_event_exp_7d_half_life_time_decay_weight,
    "marketing_attribution_event_id" as marketing_attribution_event_id,
    marketing_event_application_id
    FROM (SELECT * FROM `//home/cloud_analytics/marketing/attribution/attribution_flat`) as attr
    LEFT JOIN (SELECT * FROM `//home/cloud_analytics/ba_tags/events`) as events
    ON attr.marketing_attribution_event_id = events.event_id
    WHERE marketing_event_application_id is not null
) as ba_weight
ON events_stats.participant_billing_account_id = ba_weight.billing_account_id
AND events_stats.application_id = ba_weight.marketing_event_application_id
LEFT JOIN (
    SELECT 
        billing_account_id,
        sum(real_consumption_vat) as paid
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    GROUP BY billing_account_id
) as ba_cons
ON events_stats.participant_billing_account_id = ba_cons.billing_account_id

END DEFINE;
EXPORT $events_stats_temp;