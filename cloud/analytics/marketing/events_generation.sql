use hahn;

-- PRAGMA yt.Pool = 'cloud_analytics_pool'; 

--PRAGMA Library('time.sql');
IMPORT time SYMBOLS $format_date, $parse_date, $format_datetime, $parse_datetime;


DEFINE subquery $puid_ba_cloud() AS (
SELECT 
    puid,
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(cloud_id), ',') as cloud_id, 
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(billing_account_id), ',') as billing_account_id,
    MIN(cloud_cohort_w) as cloud_cohort_w,
    MIN(cloud_cohort_m) as cloud_cohort_m
FROM `//home/cloud_analytics/marketing/ba_cloud_puid`
GROUP BY 
    puid
);
END DEFINE;

DEFINE subquery $ba_cloud_puid() AS (
SELECT 
    billing_account_id,
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(puid), ',') as puid,
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(cloud_id), ',') as cloud_id,
    MIN(cloud_cohort_w) as cloud_cohort_w,
    MIN(cloud_cohort_m) as cloud_cohort_m
FROM `//home/cloud_analytics/marketing/ba_cloud_puid`
WHERE 
    billing_account_id is not null
GROUP BY 
    billing_account_id
);
END DEFINE;

DEFINE subquery $cloud_ba_puid() AS (
SELECT 
    cloud_id,
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(billing_account_id), ',') as billing_account_id,
    String::JoinFromList(AGGREGATE_LIST_DISTINCT(puid), ',') as puid,
    MIN(cloud_cohort_w) as cloud_cohort_w,
    MIN(cloud_cohort_m) as cloud_cohort_m
FROM `//home/cloud_analytics/marketing/ba_cloud_puid`
GROUP BY 
    cloud_id
);
END DEFINE;

$match_marketing_event_utm = Re2::Match('.*_EVNT_.*');

DEFINE subquery $visits() AS (
SELECT 
    visit_id,
    puid,
    cloud_id, 
    billing_account_id,
    event_type,
    event_date,
    event_time,
    referer,
    counter_id,
    start_url,
    trafic_source,
    utm_source,
    utm_content,
    utm_campaign,
    utm_campaign_name,
    String::SplitToList(utm_campaign_name, '_')[0] as utm_campaign_name_1,
    String::SplitToList(utm_campaign_name, '_')[1] as utm_campaign_name_2,
    String::SplitToList(utm_campaign_name, '_')[2] as utm_campaign_name_3,
    String::SplitToList(utm_campaign_name, '_')[3] as utm_campaign_name_4,
    String::SplitToList(utm_campaign_name, '_')[4] as utm_campaign_name_5,
    String::SplitToList(utm_campaign_name, '_')[5] as utm_campaign_name_6,
    String::SplitToList(utm_campaign_name, '_')[6] as utm_campaign_name_7,
    utm_campaign_id,
    utm_medium,
    utm_term,   
    IF($match_marketing_event_utm(utm_campaign), 'Marketing Events', channel) as channel,
    IF($match_marketing_event_utm(utm_campaign), 'Marketing', channel_marketing_influenced) as channel_marketing_influenced
FROM (
SELECT 
    --UTMSource, UTMContent, UTMCampaign, UTMMedium, UTMTerm
    visit_id,
    visits_raw.puid as puid,
    cloud_id, 
    billing_account_id,
    'visit' as event_type,
    event_date,
    event_time,
    referer,
    counter_id,
    start_url,
    trafic_source,
    utm_source,
    utm_content,
    utm_campaign,
    String::SplitToList(utm_campaign, '|')[0] as utm_campaign_name,
    String::SplitToList(utm_campaign, '|')[0] as utm_campaign_id,
    utm_medium,
    utm_term,
    COALESCE(utm_source_medium_channel.channel, trafic_source_channel.channel) as channel,
    COALESCE(utm_source_medium_channel.channel_marketing_influenced, trafic_source_channel.channel_marketing_influenced) as channel_marketing_influenced,
FROM (
    SELECT 
        VisitID as visit_id,
        UTCStartTime as event_time,
        $format_date(DateTime::FromSeconds(UTCStartTime)) as event_date,
        CAST(PassportUserID AS String) as puid,
        Referer as referer,
        CounterID as counter_id,
        StartURL as start_url,
        COALESCE(CAST(TraficSourceID AS String),'0') as trafic_source,
        UTMSource as utm_source, 
        UTMContent as utm_content, 
        UTMCampaign as utm_campaign, 
        UTMMedium as utm_medium, 
        UTMTerm as utm_term
    FROM RANGE('//home/cloud_analytics/import/metrika')
    GROUP BY 
        StartDate,
        VisitID,
        UTCStartTime,
        PassportUserID,
        Referer,
        CounterID,
        StartURL,
        TraficSourceID,
        UTMSource, 
        UTMContent, 
        UTMCampaign, 
        UTMMedium, 
        UTMTerm
    HAVING sum(Sign)>0
) as visits_raw
LEFT JOIN $puid_ba_cloud() as puid_ba_cloud
ON visits_raw.puid = puid_ba_cloud.puid
LEFT JOIN `//home/cloud_analytics/marketing/channel_matching/utm_source_medium` as utm_source_medium_channel
ON COALESCE(visits_raw.utm_source, '') = COALESCE(utm_source_medium_channel.UTMSource,'') AND COALESCE(visits_raw.utm_medium,'') = COALESCE(utm_source_medium_channel.UTMMedium,'')
LEFT JOIN `//home/cloud_analytics/marketing/channel_matching/trafic_source` as trafic_source_channel
ON visits_raw.trafic_source = trafic_source_channel.TraficSourceID
)
);
END DEFINE;

DEFINE subquery $cloud_created_events() AS (
    SELECT 
        cloud.cloud_id as cloud_id,
        billing_account_id,
        puid,
        'cloud_created' as event_type,
        $format_date(DateTime::ParseIso8601(cloud_created_at)) as event_date,
        DateTime::ToSeconds(DateTime::MakeTimestamp(DateTime::ParseIso8601(cloud_created_at))) as event_time
    FROM     
        `//home/cloud_analytics/import/iam/cloud_owners_history` as cloud
    LEFT JOIN $cloud_ba_puid() as cloud_ba_puid
    ON cloud.cloud_id = cloud_ba_puid.cloud_id
);
END DEFINE;

DEFINE subquery $ba_created_events() AS (
    SELECT DISTINCT
        id as billing_account_id,
        cloud_id,
        puid,
        'ba_created' AS event_type,
        $format_date(DateTime::FromSeconds(CAST(created_at AS Uint32))) AS event_date,
        created_at as event_time,
    FROM `//home/cloud/billing/exported-billing-tables/billing_accounts_prod` as ba 
    LEFT JOIN $ba_cloud_puid() as ba_cloud_puid
    ON ba.id = ba_cloud_puid.billing_account_id
);
END DEFINE;

DEFINE subquery $first_trial_cons_events() AS (
SELECT *
FROM (
    SELECT DISTINCT
        ba_cube.billing_account_id as billing_account_id,
        cloud_id,
        puid,
        'first_trial_cons' AS event_type,
        MIN(if(billing_record_monetary_grant_credit_charge>0,billing_record_date, '2099-12-31')) as event_date,
        MIN(if(billing_record_monetary_grant_credit_charge>0,billing_record_start_time, 4102433999)) + 3600 as event_time --2099-12-31 23:59:59
    FROM `//home/cloud/billing/analytics_cube/realtime/prod_full` as ba_cube
    LEFT JOIN $ba_cloud_puid() as ba_cloud_puid
    ON ba_cube.billing_account_id = ba_cloud_puid.billing_account_id
    GROUP BY 
        ba_cube.billing_account_id,
        ba_cloud_puid.cloud_id,
        ba_cloud_puid.puid
)
    WHERE event_date != '2099-12-31'
);
END DEFINE;


DEFINE subquery $first_paid_cons_events() AS (
SELECT *
FROM (
    SELECT DISTINCT
        ba_cube.billing_account_id as billing_account_id,
        cloud_id,
        puid,
        'first_paid_cons' AS event_type,
        MIN(if(billing_record_total>0,billing_record_date, '2099-12-31')) as event_date,
        MIN(if(billing_record_total>0,billing_record_start_time, 4102433999)) + 3600 as event_time --2099-12-31 23:59:59
    FROM `//home/cloud/billing/analytics_cube/realtime/prod_full` as ba_cube
    LEFT JOIN $ba_cloud_puid() as ba_cloud_puid
    ON ba_cube.billing_account_id = ba_cloud_puid.billing_account_id
    GROUP BY 
        ba_cube.billing_account_id,
        ba_cloud_puid.cloud_id,
        ba_cloud_puid.puid,
        ba_cloud_puid.cloud_cohort_w,
        ba_cloud_puid.cloud_cohort_m
)
WHERE event_date != '2099-12-31'
);
END DEFINE;

DEFINE subquery $marketing_events_registration_events() AS (
SELECT
    application_id as marketing_event_application_id,
    event_id as marketing_event_id,
    application_datetime as event_time,
    $format_date(DateTime::FromSeconds(CAST(application_datetime AS Uint32))) as event_date,
    'marketing_event_application' as event_type,
    CAST(marketing_events_stats.participant_puid AS String) as puid,
    cloud_id,
    billing_account_id,
    'Marketing Events' AS channel,
    'Marketing' as channel_marketing_influenced
FROM `//home/cloud_analytics/marketing/events/events_stats` as marketing_events_stats
LEFT JOIN $puid_ba_cloud() as puid_ba_cloud
ON CAST(marketing_events_stats.participant_puid AS String) = puid_ba_cloud.puid
);
END DEFINE;

--INSERT INTO `//home/cloud_analytics/ba_tags/events` WITH TRUNCATE
DEFINE action $get_events($path) AS
    $events = (
    SELECT * FROM $visits()
    
    UNION ALL
    
    SELECT * FROM $cloud_created_events()
    
    UNION ALL 
    
    SELECT * FROM $ba_created_events()
    
    UNION ALL 
    
    SELECT * FROM $first_trial_cons_events()
    
    UNION ALL 
    
    SELECT * FROM $first_paid_cons_events()
    
    UNION ALL 
    
    SELECT * FROM $marketing_events_registration_events()
    );
    INSERT INTO $path WITH TRUNCATE
    SELECT 
        CASE 
            WHEN event_type = 'visit' 
                THEN Digest::CityHash(event_type || CAST(event_time AS String) || CAST(visit_id AS String))
            WHEN event_type = 'marketing_event_application' 
                THEN Digest::CityHash(event_type || CAST(event_time AS String) || CAST(marketing_event_application_id AS String))
            WHEN event_type = 'cloud_created' 
                THEN Digest::CityHash(event_type || CAST(event_time AS String) || cloud_id)
            WHEN event_type in ('ba_created','first_trial_cons','first_paid_cons') 
                THEN Digest::CityHash(event_type || CAST(event_time AS String) || billing_account_id)
            ELSE 0
        END as event_id,
        billing_account_id,
        cloud_id,
        puid,
        event_type,
        event_date,
        event_time,
        referer,
        counter_id,
        visit_id,
        start_url,
        trafic_source,
        utm_source,
        utm_content,
        utm_campaign,
        utm_campaign_name,
        utm_campaign_name_1,
        utm_campaign_name_2,
        utm_campaign_name_3,
        utm_campaign_name_4,
        utm_campaign_name_5,
        utm_campaign_name_6,
        utm_campaign_name_7,
        utm_medium,
        utm_term,
        channel,
        channel_marketing_influenced,
        marketing_event_id,
        marketing_event_application_id
    FROM $events as events
END DEFINE;

EXPORT $get_events;
