PRAGMA Library("tables.sql");

IMPORT `tables` SYMBOLS $concat_path;

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

$start_of_history = {{ param["start_of_history"] -> quote() }};

$ts_parse = DateTime::Parse("%Y-%m-%dT%H:%M:%SZ");
$updates_start_dt = CAST(CurrentUtcDate() - DateTime::IntervalFromDays(1) as DateTime);


$already_loaded_tables = (
    SELECT TableName(Path)
    FROM (
        SELECT
            $ts_parse(Yson::YPathString(Attributes, '/modification_time')) as modified_at_dt,
            `Type`,
            `Path`
        FROM FOLDER($dst_folder, 'modification_time')
    )
    WHERE `Type`='table'
);

$all_metrika_tables = (
    SELECT
        TableName(Path) AS table_name,
        modified_at_dt  AS modified_at_dt
    FROM (
        SELECT
            $ts_parse(Yson::YPathString(Attributes, '/modification_time')) as modified_at_dt,
            `Type`,
            `Path`
        FROM FOLDER($src_folder, 'modification_time')
    )
    WHERE `Type`='table'
);

$actual_metrika_tables = (
    SELECT table_name, modified_at_dt
    FROM $all_metrika_tables
    WHERE table_name >= $start_of_history
);

$tables_to_load = (
    SELECT AGGREGATE_LIST(table_name)
    FROM (
        SELECT DISTINCT table_name
        FROM (
            SELECT table_name
            FROM $actual_metrika_tables
            WHERE modified_at_dt > $updates_start_dt

            UNION ALL

            SELECT table_name
            FROM (
                SELECT table_name
                FROM $actual_metrika_tables
                WHERE table_name NOT IN $already_loaded_tables
                ORDER BY table_name DESC
                LIMIT 50
            )
        ) AS t
    ) AS t
);


DEFINE ACTION $insert_partition($date) AS
    $source_table = $concat_path($src_folder, $date);
    $destination_table = $concat_path($dst_folder, $date);

   INSERT INTO $destination_table WITH TRUNCATE
    SELECT
           BrowserCountry                                    as browser_country,
           BrowserEngineID                                   as browser_engine_id,
           BrowserEngineVersion1                             as browser_engine_version1,
           BrowserEngineVersion2                             as browser_engine_version2,
           BrowserEngineVersion3                             as browser_engine_version3,
           BrowserEngineVersion4                             as browser_engine_version4,
           BrowserLanguage                                   as browser_language,
           CounterID                                         as counter_id,
           CryptaID                                          as crypta_id,
           DeviceModel                                       as device_model,
           DevicePixelRatio                                  as device_pixel_ratio,
           DomainZone                                        as domain_zone,
           Duration                                          as duration,
           EndURL                                            as end_url,
           Event_ID                                          as event_id,
           Event_InvolvedTime                                as event_involved_time,
           Event_IsPageView                                  as event_is_page_view,
           FirstVisit                                        as FirstVisit,
           GoalReachesAny                                    as goal_reaches_any,
           Goals_EventTime                                   as goals_event_time,
           Goals_ID                                          as goals_id,
           Hits                                              as hits,
           IsBounce                                          as is_bounce,
           IsDownload                                        as is_download,
           IsLoggedIn                                        as is_logged_in,
           IsMobile                                          as is_mobile,
           IsPrivateMode                                     as is_private_mode,
           IsRobot                                           as is_robot,
           IsTV                                              as is_tv,
           IsTablet                                          as is_tablet,
           IsTurboPage                                       as is_turbo_page,
           IsWebView                                         as is_web_view,
           LastVisit                                         as last_visit,
           LinkURL                                           as link_url,
           MobilePhoneModel                                  as mobile_phone_model,
           MobilePhoneVendor                                 as mobile_phone_vendor,
           OS                                                as os,
           OSFamily                                          as os_family,
           OSName                                            as os_name,
           ParsedParams_Key1                                 as parsed_params_key_1,
           ParsedParams_Key10                                as parsed_params_key_10,
           ParsedParams_Key2                                 as parsed_params_key_2,
           ParsedParams_Key3                                 as parsed_params_key_3,
           ParsedParams_Key4                                 as parsed_params_key_4,
           ParsedParams_Key5                                 as parsed_params_key_5,
           ParsedParams_Key6                                 as parsed_params_key_6,
           ParsedParams_Key7                                 as parsed_params_key_7,
           ParsedParams_Key8                                 as parsed_params_key_8,
           ParsedParams_Key9                                 as parsed_params_key_9,
           PassportUserID                                    as puid,
           PublisherEvents_AdvEngineID                       as publisher_events_adv_engine_id,
           PublisherEvents_ArticleHeight                     as publisher_events_article_height,
           PublisherEvents_ArticleID	                     as publisher_events_article_id,
           PublisherEvents_Authors		                     as publisher_events_authors,
           PublisherEvents_Chars		                     as publisher_events_chars,
           PublisherEvents_EventID		                     as publisher_events_event_id,
           PublisherEvents_FromArticleID                     as publisher_events_from_article_id,
           PublisherEvents_HasRecircled	                     as publisher_events_has_recircled,
           PublisherEvents_HitEventTime	                     as publisher_events_hit_event_time,
           PublisherEvents_InvolvedTime	                     as publisher_events_involved_time,
           PublisherEvents_MessengerID		                 as publisher_events_messenger_id,
           PublisherEvents_PublicationTime		             as publisher_events_publication_time,
           PublisherEvents_RecommendationSystemID		     as publisher_events_recommendation_system_id,
           PublisherEvents_ReferrerDomain		             as publisher_events_referrer_domain,
           PublisherEvents_ReferrerPath                      as publisher_events_referrer_path,
           PublisherEvents_Rubric		                     as publisher_events_rubric,
           PublisherEvents_Rubric2		                     as publisher_events_rubric_2,
           PublisherEvents_ScrollDown	                     as publisher_events_scroll_down,
           PublisherEvents_SearchEngineID		             as publisher_events_search_engine_id,
           PublisherEvents_SocialSourceNetworkID		     as publisher_events_social_source_network_id,
           PublisherEvents_Title		                     as publisher_events_title,
           PublisherEvents_Topics		                     as publisher_events_topics,
           PublisherEvents_TraficSource                      as publisher_events_trafic_source,
           PublisherEvents_TurboType	                     as publisher_events_turbo_type,
           PublisherEvents_URLCanonical                      as publisher_events_url_canonical,
           Referer                                           as referer,
           RefererDomain                                     as referer_domain,
           RegionID                                          as region_id,
           ResolutionDepth                                   as resolution_depth,
           ResolutionHeight                                  as resolution_height,
           ResolutionWidth                                   as resolution_width,
           SearchEngineID                                    as search_engine_id,
           SearchPhrase                                      as search_phrase,
           SocialSourceNetworkID                             as social_source_network_id,
           StartDate                                         as start_date,
           StartURL                                          as start_url,
           StartURLDomain                                    as start_url_domain,
           TopLevelDomain                                    as top_level_domain,
           TotalVisits                                       as total_visits,
           COALESCE(TraficSourceID, 0)                       as trafic_source_id,
           UTCStartTime                                      as event_start_dt_utc,
           UTMCampaign                                       as utm_campaign,
           UTMContent                                        as utm_content,
           UTMMedium                                         as utm_medium,
           UTMSource                                         as utm_source,
           UTMTerm                                           as utm_term,
           UserAgent                                         as user_agent,
           UserAgentMajor                                    as user_agent_major,
           UserAgentMinor                                    as user_agent_minor,
           UserAgentVersion2                                 as user_agent_version_2,
           UserAgentVersion3                                 as user_agent_version_3,
           UserAgentVersion4                                 as user_agent_version_4,
           UserID                                            as user_id,
           UserIDType                                        as user_id_type,
           VisitID                                           as visit_id,
           WindowClientHeight                                as window_client_height,
           WindowClientWidth                                 as window_client_width,
           YandexLogin                                       as yandex_login,
    FROM $source_table
    WHERE CounterID in (50027884, 48570998, 51465824, 50514217, 43681409)
    ORDER BY visit_id, counter_id, event_start_dt_utc
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
