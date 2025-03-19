PRAGMA Library("attribution_functions.sql");
PRAGMA Library("datetime.sql");
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

IMPORT attribution_functions SYMBOLS $apply_attribution_all;
IMPORT `datetime` SYMBOLS $get_msk_datetime;

$dm_marketing_events = {{ param["dm_marketing_events"] -> quote() }};
$visit_log_folder = {{ param["visit_log_folder"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$CHANNEL_DIRECT = 'Direct';
$CHANNEL_EMAILING = 'Emailing';
$CHANNEL_EVENTS = 'Events';
$CHANNEL_MESSENGERS = 'Messengers';
$CHANNEL_ORGANIC_SEARCH = 'Organic Search';
$CHANNEL_OTHER = 'Other';
$CHANNEL_PERFORMANCE = 'Performance';
$CHANNEL_RECOMMENDER_SYSTEMS = 'Recommender Systems';
$CHANNEL_REFERRALS = 'Referrals';
$CHANNEL_SITE_OTHERS = 'Site Others';
$CHANNEL_SITE_PROMO = 'Site and Console Promo';
$CHANNEL_SOCIAL_OWNED = 'Social Owned';
$CHANNEL_SOCIAL_ORGANIC = 'Social (organic)';
$CHANNEL_UNKNOWN = 'Unknown';
$CHANNEL_YANDEX_PORTAL = 'Yandex Portal';
$CHANNEL_YC_APP = 'YC App';

$resolve_traffic_source_name = ($traffic_source_id) -> (
    CASE
        WHEN $traffic_source_id =  0  THEN $CHANNEL_DIRECT
        WHEN $traffic_source_id =  2  THEN $CHANNEL_ORGANIC_SEARCH
        WHEN $traffic_source_id =  1  THEN $CHANNEL_REFERRALS
        WHEN $traffic_source_id = -1  THEN $CHANNEL_SITE_OTHERS
        WHEN $traffic_source_id =  3  THEN $CHANNEL_PERFORMANCE
        WHEN $traffic_source_id =  8  THEN $CHANNEL_SOCIAL_ORGANIC
        WHEN $traffic_source_id =  10 THEN $CHANNEL_MESSENGERS
        WHEN $traffic_source_id =  9  THEN $CHANNEL_REFERRALS
        WHEN $traffic_source_id =  5  THEN $CHANNEL_UNKNOWN
        WHEN $traffic_source_id =  7  THEN $CHANNEL_OTHER
        WHEN $traffic_source_id =  4  THEN $CHANNEL_DIRECT
        ELSE $CHANNEL_UNKNOWN
    END
);

$resolve_channel_marketing_influenced = ($channel) -> (
    CASE  
        WHEN $channel in ($CHANNEL_EMAILING, $CHANNEL_EVENTS, $CHANNEL_ORGANIC_SEARCH,
            $CHANNEL_PERFORMANCE, $CHANNEL_SITE_PROMO, $CHANNEL_SOCIAL_OWNED, 
            $CHANNEL_YANDEX_PORTAL)                                                                 THEN 'Marketing'
        WHEN $channel in ($CHANNEL_UNKNOWN, 'Unmatched')                                            THEN 'Unmatched'
                                                                                                    ELSE 'Non-Marketing'
    END
);

$resolve_chanel = ($trafic_source_id, $utm_source, $utm_medium) -> (     
    CASE 
        WHEN $utm_source = 'ycloudapp'                                                    THEN $CHANNEL_YC_APP
        WHEN $utm_source = 'koldunshik'                                                   THEN $CHANNEL_YANDEX_PORTAL
        WHEN $utm_source = 'mail' and $utm_medium = 'kassa_auto'                          THEN $CHANNEL_YANDEX_PORTAL
        WHEN $utm_source = 'tg_from_scale2020' and $utm_medium = ''                       THEN $CHANNEL_MESSENGERS
        WHEN $utm_source = 'youtube' and $utm_medium = ''                                 THEN $CHANNEL_PERFORMANCE
        WHEN $utm_source = 'ds2020' and $utm_medium = 'email'                             THEN $CHANNEL_PERFORMANCE
        WHEN $utm_source = 'tproger' and $utm_medium = 'post'                             THEN $CHANNEL_MESSENGERS
        WHEN $utm_source = 'choose-your-db' and $utm_medium = 'referral'                  THEN $CHANNEL_MESSENGERS
        WHEN $utm_source = 'habr' and $utm_medium = 'test'                                THEN $CHANNEL_REFERRALS
        WHEN $utm_medium IN ('yacloudnews', 'yacloudchannel','yacloudsource')             THEN $CHANNEL_SOCIAL_OWNED
        WHEN $utm_source = 'tp_innopolis' and $utm_medium = 'email'                       THEN $CHANNEL_UNKNOWN
        WHEN $utm_source IN ('amp_eskimobi', 'exs', 'share2')                             THEN $CHANNEL_UNKNOWN
        WHEN $utm_source IN ('turbo_turbo','innopolis','promo','doodle','sprav',
            'astana hub','lkmarket','y4dt','ctoschool')
            and $utm_medium = ''                                                          THEN $CHANNEL_UNKNOWN
        WHEN $utm_source IN ('webasyst','direct','Biggest-Web-Hosting-Directory',
            'tp_innopolis')
            and $utm_medium IN ('mylang','share','Listed-on-WHTop.com',
            'empty-page','email')                                                         THEN $CHANNEL_UNKNOWN
        WHEN $utm_source IN ('google, yandex', 'yandex', 'google')  
            AND ($utm_medium IS NULL OR $utm_medium IN ('', 'organic', 'search'))         THEN $CHANNEL_ORGANIC_SEARCH
        WHEN $utm_medium = 'tg'                                                           THEN $CHANNEL_SOCIAL_ORGANIC
        WHEN $utm_medium = 'press'                                                        THEN $CHANNEL_REFERRALS

        WHEN $utm_medium IN ('cpc', 'paidpost', 'cpm','timepad',
                        'test','sponsoredemail','security','paidvideo','gdn',
                        '300120','landing_page',
                        'display','banner_240x400')   
            OR $utm_source IN ('CloudEvents','linkedin','ds2020_aic')                        
            OR $utm_source REGEXP 'Display' 
            OR $utm_medium REGEXP 'Display'
            OR $utm_source REGEXP 'yadirect'
            OR $utm_source REGEXP 'Video_Cloud'                                         THEN $CHANNEL_PERFORMANCE
        WHEN $utm_source IN  ('facebook','fb','tw','vk', 'ok',
            'twitter.com', 'facebook.com', 
            'vk.com')
            OR $utm_medium = 'smm' 
            OR $utm_source REGEXP 'from_scale'                                          THEN $CHANNEL_SOCIAL_ORGANIC

        WHEN $utm_source IN ('t.me/ppc_analytics','telegram',
                             'telergam','tg', 'telegram.me', 
                             'telegram.me')                                               THEN $CHANNEL_MESSENGERS
        WHEN $utm_medium = 'dialogsalice'                                                 THEN $CHANNEL_REFERRALS
        WHEN $utm_source IN ('bno','koldunzhik','objectanswer',
                             'teasermain','yandex','yaserp',
                             'ymainteaser','aid','yamainbanner',
                             'yamainbanner_mobile','yamainbrowser',
                             'connect_promo','yametrika',
                             'yandex.metrika','wmblog','dialogsblog','ywmmenu',
                             'yandex.webmaster','tech.yandex.ru','yandexb2b',
                             'yandex.business','yandexkassa',
                             'main','yandex_webmaster','dialogs','chplayground',
                             'vmblog','zenblog',
                             'webmaster','academy.yandex.ru','disk','techblog'
                             ,'techyandexru')                                               THEN $CHANNEL_YANDEX_PORTAL
        WHEN $utm_source IN ('dialogsblog','habr', 'habrahabr',
                             'referrals','Searchengines.ru',
                             'tech.yandex.ru','techyandexru','vc',
                             'wmblog','yametrika',
                             'yametrika','ywmmenu', 'tproger_ru', 
                             'zen', 'techblog', 'ywebmasterblog', 
                             'sender', 'r-point', 'retail_conf', 
                             'public-embed','school','partner','vc.ru',
                             'habr','tagline','knopka')                                     THEN  $CHANNEL_REFERRALS

        WHEN ($utm_source  REGEXP 'email(ing)?(s)?'  
            OR $utm_medium  REGEXP 'email(ing)?(s)?' )                                      THEN $CHANNEL_EMAILING                 
        WHEN $utm_medium IN ('scale2021','banner','referral',
            'links','cloud_boost','topright','landing') 
            OR $utm_source IN ('cloud.yandex','blog-site-link')
            OR $utm_source REGEXP 'banner' 
            OR $utm_source REGEXP 'console'    
            OR $utm_medium REGEXP 'banner'  
            OR $utm_medium REGEXP 'footer'                                                  THEN $CHANNEL_SITE_PROMO


        WHEN $trafic_source_id == -1                                THEN $CHANNEL_SITE_OTHERS
        WHEN $trafic_source_id ==  0                                THEN $CHANNEL_DIRECT
        WHEN $trafic_source_id ==  1                                THEN $CHANNEL_REFERRALS
        WHEN $trafic_source_id ==  2                                THEN $CHANNEL_ORGANIC_SEARCH
        WHEN $trafic_source_id ==  4                                THEN $CHANNEL_DIRECT
        WHEN $trafic_source_id ==  5                                THEN $CHANNEL_UNKNOWN
        WHEN $trafic_source_id ==  8                                THEN $CHANNEL_SOCIAL_ORGANIC
        WHEN $trafic_source_id ==  9                                THEN $CHANNEL_RECOMMENDER_SYSTEMS
        WHEN $trafic_source_id == 10                                THEN $CHANNEL_MESSENGERS

        ELSE $CHANNEL_OTHER
    
    END
);

$marketing_events = (
    SELECT
        event_type,
        event_id,
        billing_account_id,
        event_time_msk,
        puid 
    FROM $dm_marketing_events
    WHERE event_type in (
        'billing_account_created',
        'event_application',
        'event_visit',
        'site_visit'
    )
);

$visits = (
    SELECT
        visit_id,
        utm_source,
        utm_medium,
        utm_campaign,
        utm_content,
        utm_term,
        trafic_source_id,
        traffic_source_name,
        channel,
        $resolve_channel_marketing_influenced(channel)  AS channel_marketing_influenced
    FROM (
        SELECT
               CAST(visit_id as STRING)                                     AS visit_id,
               utm_source,
               utm_medium,
               utm_campaign,
               utm_content,
               utm_term,
               trafic_source_id,
               $resolve_traffic_source_name(trafic_source_id)               AS traffic_source_name,
               $resolve_chanel(trafic_source_id, utm_source, utm_medium)    AS channel,            
        FROM RANGE($visit_log_folder)
    ) AS t
);

$extract_campaign = ($utm_campaign, $idx) -> {
    $list = String::SplitToList($utm_campaign, '_');

    RETURN coalesce(IF( ListLength($list) = 7, $list[$idx], ""), "")
};

$offline_events = (
    SELECT
        event_type          AS event_type,
        event_id            AS event_id,
        billing_account_id  AS billing_account_id,
        event_time_msk      AS event_time_msk,
        puid                AS puid,
        $CHANNEL_EVENTS     AS channel,
        'Marketing'         AS marketing_influenced
    FROM $marketing_events
    WHERE event_type in (
        'event_application',
        'event_visit'
    )
);

$marketing_events = (
    SELECT
        events.billing_account_id as billing_account_id,
        (
            coalesce(events.event_id, ""),
            events.event_type,
            coalesce(CAST(events.event_time_msk AS Uint64), 0),
            coalesce(visits.channel, offline_events.channel, ""),
            coalesce(visits.channel_marketing_influenced, offline_events.marketing_influenced, ""),
            coalesce(visits.utm_campaign, ""),
            $extract_campaign(visits.utm_campaign, 0),
            $extract_campaign(visits.utm_campaign, 1),
            $extract_campaign(visits.utm_campaign, 2),
            $extract_campaign(visits.utm_campaign, 3),
            $extract_campaign(visits.utm_campaign, 4),
            $extract_campaign(visits.utm_campaign, 5),
            $extract_campaign(visits.utm_campaign, 6),
            coalesce(visits.utm_source, ""),
            coalesce(visits.utm_medium, ""),
            coalesce(visits.utm_content, ""),
            coalesce(visits.utm_term, ""),
        ) as event
    FROM $marketing_events AS events
        LEFT JOIN $visits AS visits
            ON events.event_id = visits.visit_id
        LEFT JOIN $offline_events AS offline_events
            ON events.event_id = offline_events.event_id
    WHERE events.billing_account_id IS NOT NULL

);

$result = (
    SELECT
        billing_account_id                    as billing_account_id,
        attribution.0                         as event_id,
        attribution.1                         as event_type,
        $get_msk_datetime(attribution.2)      as event_datetime_msk,
        attribution.3                         as channel,
        attribution.4                         as channel_marketing_influenced,
        attribution.5                         as utm_campaign_name,
        attribution.6                         as utm_campaign_traffic_soure,
        attribution.7                         as utm_campaign_country,
        attribution.8                         as utm_campaign_city,
        attribution.9                         as utm_campaign_device,
        attribution.10                        as utm_campaign_budget,
        attribution.11                        as utm_campaign_product,
        attribution.12                        as utm_campaign_business_unit,
        attribution.13                        as utm_source,
        attribution.14                        as utm_medium,
        attribution.15                        as utm_content,
        attribution.16                        as utm_term,
        attribution.17[0]                     as event_first_weight,
        attribution.17[1]                     as event_last_weight,
        attribution.17[2]                     as event_uniform_weight,
        attribution.17[3]                     as event_u_shape_weight,
        attribution.17[4]                     as event_exp_7d_half_life_time_decay_weight,
    FROM (
        SELECT billing_account_id,
            $apply_attribution_all(
                    ListSortAsc(AGGREGATE_LIST(event), ($x) -> {RETURN $x.2}),
                    'billing_account_created'
            ) as attribution
        FROM $marketing_events
        GROUP BY billing_account_id
    )
    FLATTEN BY attribution
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT *
FROM $result
ORDER BY billing_account_id;
