PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $concat_path;
IMPORT `tables` SYMBOLS $get_daily_tables_to_load;

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

$cluster = {{ cluster -> table_quote() }};
$start_of_history =  {{ param["start_of_history"] -> quote() }};
$reload_before_datetime = {{ param["reload_before_datetime"] -> quote() }};
$days_qty_to_reload =  {{ param["days_qty_to_reload"] }};
$limit = {{ param["limit"] }};

$limit = UNWRAP($limit);
$reload_before_datetime = CAST(date($reload_before_datetime) AS DateTime);


$tables_to_load = (
    SELECT *
    FROM $get_daily_tables_to_load($cluster, $src_folder, $dst_folder, $start_of_history, $days_qty_to_reload, $reload_before_datetime, $limit)
);

DEFINE ACTION $insert_partition($date) AS
    $source_table = $concat_path($src_folder, $date);
    $destination_table = $concat_path($dst_folder, $date);

   INSERT INTO $destination_table WITH TRUNCATE
    SELECT
           BrowserCountry                                    as browser_country,
           BrowserEngineID                                   as browser_engine_id,
           BrowserEngineVersion1                             as browser_engine_version_1,
           BrowserEngineVersion2                             as browser_engine_version_2,
           BrowserEngineVersion3                             as browser_engine_version_3,
           BrowserEngineVersion4                             as browser_engine_version_4,
           BrowserLanguage                                   as browser_language,
           ClientIPNetwork                                   as client_ip_network,
           CounterID                                         as counter_id,
           CryptaID                                          as crypta_id,
           DeviceModel                                       as device_model,
           DevicePixelRatio                                  as device_pixel_ratio,
           DomainZone                                        as domain_zone,
           DontCountHits                                     as dont_count_hits,
           EventDate                                         as event_date,
           GoalsReached                                      as goals_reached,
           GoalsType                                         as goals_type,
           HasWWW                                            as has_www,
           IsArtifical                                       as is_artifical,
           IsDownload                                        as is_download,
           IsEvent                                           as is_event,
           IsIFrame                                          as is_iframe,
           IsLink                                            as is_link,
           IsLoggedIn                                        as is_logged_in,
           IsNotBounce                                       as is_not_bounce,
           IsParameter                                       as is_parameter,
           IsPrivateMode                                     as is_private_mode,
           IsRobot                                           as is_robot,
           IsTV                                              as is_tv,
           IsTablet                                          as is_tablet,
           IsTurboPage                                       as is_turbo_page,
           IsWebView                                         as is_web_view,
           MobilePhoneModel                                  as mobile_phone_model,
           MobilePhoneVendor                                 as mobile_phone_vendor,
           OS                                                as os,
           OSFamily                                          as os_family,
           OSName                                            as os_name,
           OriginalURL                                       as original_url,
           Params                                            as params,
           ParsedParams_Key1                                 as parsed_params_key1,
           ParsedParams_Key10                                as parsed_params_key10,
           ParsedParams_Key2                                 as parsed_params_key2,
           ParsedParams_Key3                                 as parsed_params_key3,
           ParsedParams_Key4                                 as parsed_params_key4,
           ParsedParams_Key5                                 as parsed_params_key5,
           ParsedParams_Key6                                 as parsed_params_key6,
           ParsedParams_Key7                                 as parsed_params_key7,
           ParsedParams_Key8                                 as parsed_params_key8,
           ParsedParams_Key9                                 as parsed_params_key9,
           PassportUserID                                    as puid,
           Referer                                           as referer,
           RefererDomain                                     as referer_domain,
           Refresh                                           as refresh,
           RegionID                                          as region_id,
           ResolutionDepth                                   as resolution_depth,
           ResolutionHeight                                  as resolution_height,
           ResolutionWidth                                   as resolution_width,
           SearchEngineID                                    as search_engine_id,
           SearchPhrase                                      as search_phrase,
           ShareService                                      as share_service,
           ShareTitle                                        as share_title,
           ShareURL                                          as share_url,
           SocialSourceNetworkID                             as social_source_network_id,
           SocialSourcePage                                  as social_source_page,
           Title                                             as title,
           COALESCE(TraficSourceID, 0)                       as trafic_source_id,
           URL                                               as url,
           URLDomain                                         as url_domain,
           UTCEventTime                                      as event_start_dt_utc,
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
           WatchID                                           as hit_id,
           WindowClientHeight                                as window_client_height,
           WindowClientWidth                                 as window_Client_width,
           YandexLogin                                       as yandex_login,
    FROM $source_table
    WHERE CounterID in (50027884, 48570998, 51465824, 50514217, 43681409)
    ORDER BY hit_id, counter_id, event_start_dt_utc
END DEFINE;

EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
