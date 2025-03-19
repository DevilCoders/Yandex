USE hahn;

PRAGMA AnsiInForEmptyOrNullableItemsCollections;
------------------------- PATH ------------------------------------------
$hit_log_folder = '//home/cloud-dwh/data/prod/ods/metrika/hit_log';
$visit_log_folder = '//home/cloud-dwh/data/prod/ods/metrika/visit_log';
$dl_daily_usage_path =  '//home/cloud_analytics/data_swamp/projects/datalens/datalens_daily_usage_without_ba';
$passport_users_path = '//home/cloud-dwh/data/prod/ods/iam/passport_users';
$result_table_path = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_events';
----------------------- FUNCTION ----------------------------------------
$format_d = DateTime::Format("%Y-%m-%d");
$format_dt = DateTime::Format("%Y-%m-%d %H:%M:%S");
-- -- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
-- -- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
-- -- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$msk_date = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_d));
$msk_datetime = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_dt));
------------------------- INFO ------------------------------------------
-- Счетчики:
-- 50514217 - Счетчик содержит данные внешнего DataLens (не yandex-team).
-- 51465824 - Сквозной счетчик для всех сервисов Яндекс.Облака.
-- Цели:
-- DL_INIT_WELCOME_SHOW - 231888290 - полностью новые пользователи
-- DL_INIT-CREATE-AND-ACTIVATE_CLICK - 231888276 - пользователи, создающий организацию и DataLens
-- DL_INIT_OPEN-DL_CLICK - 231888259 - пользователи, создающих (открывающих) DataLens в организации, в которой его еще нет
-- DL_PROMO_SHOW - 231884342 - Показ промо-страницы новому пользователю 
----------------------- CHANNELS ----------------------------------------
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
----------------------- FUNCTIONS --------------------------------
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
        WHEN $channel in ($CHANNEL_EMAILING, $CHANNEL_EVENTS, 
        --  $CHANNEL_ORGANIC_SEARCH,
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
----------------------- CONSTANT ----------------------------------------
-- Смотрим события с этой даты, так как в метрике появился параметр ord_id c 27/05/22
$thresh_date = '2022-05-27';
----------------------- QUERIES -----------------------------------------
DEFINE ACTION $datalens_events_script() AS
    -- Посещение стартовой страницы на datalens.yandex.ru (цель 231884342) или "https://cloud.yandex.ru/services/datalens"
   $dl_visit_goal = (
        SELECT
            visit_id
        FROM  RANGE($visit_log_folder,`2022-05-27`)
        WHERE 1=1
            AND is_robot IS NULL
            AND counter_id = 50514217
            AND ListHas(Yson::ConvertToInt64List(goals_id), 231884342)
    );

    -- Посещение стартовой страницы "https://cloud.yandex.ru/services/datalens"
    $flatten_visits = (
        SELECT *
        FROM (
            SELECT
                visit_id,
                counter_id, 
                is_robot,
                Yson::ConvertToInt64List(event_id) as event_id
            FROM  RANGE($visit_log_folder,`2022-05-27`) 
        )
        FLATTEN BY (event_id)
    );

    $dl_visit_via_cloud = (
        SELECT
            DISTINCT 
            visit_id
        FROM (
            SELECT
                v.visit_id as visit_id
            FROM $flatten_visits as v
            LEFT JOIN (
                        SELECT 
                            hit_id, 
                            url as hit_url
                        FROM  RANGE($hit_log_folder,`2022-05-27`)
                        WHERE counter_id = 51465824
                            AND StartsWith(url, "https://cloud.yandex.ru/services/datalens")
                    ) as h
                ON h.hit_id = v.event_id 
            WHERE 1=1
                AND is_robot IS NULL
                AND counter_id = 51465824
                AND StartsWith(hit_url, "https://cloud.yandex.ru/services/datalens")
        )
    );

    -- Объединяем все визиты
    $dl_visit = (
        SELECT
            metrika.visit_id as visit_id,
            start_url,
            $msk_date(event_start_dt_utc) as event_date_msk,
            $msk_datetime(event_start_dt_utc) as event_datetime_msk,
            user_id as yuid,
            puid,
            iam_uid as user_id,
            utm_source,
            utm_medium,
            utm_campaign,
            utm_content,
            utm_term,
            trafic_source_id,
            $resolve_traffic_source_name(trafic_source_id)                       AS traffic_source_name,
            $resolve_chanel(trafic_source_id, utm_source, utm_medium)            AS channel, 
            $resolve_channel_marketing_influenced(
            $resolve_chanel(trafic_source_id, utm_source, utm_medium))           AS channel_marketing_influenced
        FROM RANGE($visit_log_folder,`2022-05-27`) as metrika
        LEFT JOIN $passport_users_path as passport
            ON CAST(metrika.puid as STRING) = CAST(passport.passport_uid as STRING)
        WHERE 1=1
            AND visit_id in (select distinct visit_id from $dl_visit_goal)
            OR  visit_id in (select distinct visit_id from $dl_visit_via_cloud)
    );

    -- Создание даталенса
    $dl_create = (
        SELECT
            visit_id,
            start_url,
            $msk_date(event_start_dt_utc) as event_date_msk,
            $msk_datetime(event_start_dt_utc) as event_datetime_msk,
            CASE    
                WHEN ListHas(Yson::ConvertToStringList(parsed_params_key_1), 'org_id')
                THEN Yson::ConvertToStringList(parsed_params_key_2)[ListIndexOf(Yson::ConvertToStringList(parsed_params_key_1), 'org_id')] 
                ELSE NULL 
                END as organization_id,
            user_id as yuid,
            puid,
            iam_uid as user_id,
            utm_source,
            utm_medium,
            utm_campaign,
            utm_content,
            utm_term,
            trafic_source_id,
            $resolve_traffic_source_name(trafic_source_id)                       AS traffic_source_name,
            $resolve_chanel(trafic_source_id, utm_source, utm_medium)            AS channel, 
            $resolve_channel_marketing_influenced(
                $resolve_chanel(trafic_source_id, utm_source, utm_medium))       AS channel_marketing_influenced
        FROM  RANGE($visit_log_folder,`2022-05-27` 
                            ) as metrika
        LEFT JOIN $passport_users_path as passport
            ON CAST(metrika.puid as STRING) = CAST(passport.passport_uid as STRING)
        WHERE 1=1
            AND counter_id = 50514217
            AND is_robot IS NULL
            AND (ListHas(Yson::ConvertToInt64List(goals_id), 231888290)
                    OR ListHas(Yson::ConvertToInt64List(goals_id), 231888276)
                    OR ListHas(Yson::ConvertToInt64List(goals_id), 231888259)
                )
    );

    -- Первый запрос в организации
    $dl_active = (
        SELECT
        DISTINCT 
        FIRST_VALUE(`event_date`) OVER w as event_date_msk,
        FIRST_VALUE(`event_datetime`) OVER w as event_datetime_msk,
        FIRST_VALUE(`request_id`) OVER w as request_id,
        FIRST_VALUE(`user_id`) OVER w as user_id,
        organization_id
    FROM $dl_daily_usage_path 
    WHERE 1=1
        AND event_date >= $thresh_date
        AND organization_id is not NULL
        AND user_id != '__ANONYMOUS_USER_OF_PUBLIC_DATALENS__'
        AND user_id is not NULL
    WINDOW w AS ( -- window specification for the named window "w" used above
                PARTITION BY organization_id -- partitioning clause
                ORDER BY event_datetime ASC    -- sorting clause
                )
    );

    $dl_123 = (
            SELECT 
                visit_id,
                NULL as request_id,
                event_date_msk,
                event_datetime_msk,
                'site_visit' as event_type,
                NULL as organization_id,
                user_id,
                yuid,
                puid,
                start_url,
                utm_source,
                utm_medium,
                utm_campaign,
                utm_content,
                utm_term,
                trafic_source_id,
                traffic_source_name,
                channel, 
                channel_marketing_influenced
            FROM $dl_visit 

            UNION ALL

            SELECT 
                visit_id,
                NULL as request_id,
                event_date_msk,
                event_datetime_msk,
                'create_dl' as event_type,
                organization_id,
                user_id,
                yuid,
                puid,
                start_url,
                utm_source,
                utm_medium,
                utm_campaign,
                utm_content,
                utm_term,
                trafic_source_id,
                traffic_source_name,
                channel, 
                channel_marketing_influenced
            FROM $dl_create 

            UNION ALL
            
            SELECT 
                NULL as visit_id,
                request_id,
                event_date_msk,
                event_datetime_msk,
                'first_org_request' as event_type,
                organization_id,
                user_id,
                NULL as yuid,
                NULL as puid,
                NULL as start_url,
                NULL as utm_source,
                NULL as utm_medium,
                NULL as utm_campaign,
                NULL as utm_content,
                NULL as utm_term,
                NULL as trafic_source_id,
                NULL as traffic_source_name,
                NULL as channel, 
                NULL as channel_marketing_influenced
            FROM $dl_active 
    );

    -- В метрике могут быть дубли -> делаем дедубликацию 
    $dl_123_deduped = (select distinct
                            visit_id,
                            request_id,
                            event_date_msk,
                            event_datetime_msk,
                            event_type,
                            organization_id,
                            user_id,
                            yuid,
                            puid,
                            start_url,
                            utm_source,
                            utm_medium,
                            utm_campaign,
                            utm_content,
                            utm_term,
                            trafic_source_id,
                            traffic_source_name,
                            channel, 
                            channel_marketing_influenced
                from $dl_123
    );

    INSERT INTO $result_table_path WITH TRUNCATE
    SELECT 
        *
        FROM $dl_123_deduped
        ORDER BY yuid, event_datetime_msk
    ;

END DEFINE;

EXPORT $datalens_events_script;
