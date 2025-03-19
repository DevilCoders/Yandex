USE hahn;

PRAGMA AnsiInForEmptyOrNullableItemsCollections;
------------------------- PATH ------------------------------------------
$datalens_events = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_events';
$result_ma_path = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_marketing_attribution';
$result_funnel_path = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_funnel_with_marketing_attribution';
----------------------- FUNCTION ----------------------------------------
$format_d = DateTime::Format("%Y-%m-%d");
-- $format_dt = DateTime::Format("%Y-%m-%d %H:%M:%S");
-- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
-- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
-- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$msk_date = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_d));
-- $msk_datetime = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_dt));
-- string to datetime
$parse_datetime = DateTime::Parse("%Y-%m-%d %H:%M:%S");
$str_to_datetime = ($str) -> (DateTime::MakeDatetime($parse_datetime($str)));

-- извлечение параметров кампании из utm_campaign
$extract_campaign = ($utm_campaign, $idx) -> {
    $list = String::SplitToList($utm_campaign, '_');

    RETURN coalesce(IF( ListLength($list) = 7, $list[$idx], ""), "")
};
----------------------- QUERIES --------------------------------
DEFINE ACTION $datalens_marketing_attribution_script() AS

    $dl_events = (
        SELECT
            visit_id,
            request_id,
            event_type,
            organization_id,
            event_date_msk,
            -- CAST($str_to_datetime(event_date_msk) as Uint32) as event_date_msk,
            coalesce(CAST($str_to_datetime(event_datetime_msk) as Uint64), 0) as event_datetime_msk,
            yuid,
            puid,
            user_id,
            utm_source,
            utm_medium,
            utm_campaign,
            utm_content,
            utm_term,
            trafic_source_id,
            traffic_source_name,
            channel,
            channel_marketing_influenced
            FROM $datalens_events 
    );

    $dl_visits = (
        SELECT
            DISTINCT
            yuid,
            organization_id,
            puid,
            user_id,
            trafic_source_id,
            traffic_source_name,
            visit_id,
            event_type,
            event_date_msk,
            event_datetime_msk,
            coalesce(channel, "") as channel,
            coalesce(channel_marketing_influenced, "") as channel_marketing_influenced,
            coalesce(utm_campaign, "") as utm_campaign,
            coalesce(utm_source, "") as utm_source,
            coalesce(utm_medium, "") as utm_medium,
            coalesce(utm_content, "") as utm_content,
            coalesce(utm_term, "") as utm_term
        FROM $dl_events
        WHERE 1=1
            AND event_type = 'site_visit'
            AND yuid IS NOT NULL
    );

    $dl_create = (
        SELECT
            yuid,
            organization_id,
            puid,
            user_id,
            trafic_source_id,
            traffic_source_name,
            visit_id,
            event_type,
            event_date_msk,
            event_datetime_msk,
            coalesce(channel, "") as channel,
            coalesce(channel_marketing_influenced, "") as channel_marketing_influenced,
            coalesce(utm_campaign, "") as utm_campaign,
            coalesce(utm_source, "") as utm_source,
            coalesce(utm_medium, "") as utm_medium,
            coalesce(utm_content, "") as utm_content,
            coalesce(utm_term, "") as utm_term
        FROM $dl_events AS events
        WHERE 1=1
            AND events.event_type = 'create_dl'
            AND events.yuid IS NOT NULL
    );

    $first_request = (
        SELECT
            organization_id,
            puid,
            user_id,
            request_id,
            event_type,
            event_date_msk,
            event_datetime_msk,
        FROM $dl_events AS events
        WHERE 1=1
            AND events.event_type = 'first_org_request'
            AND events.organization_id IS NOT NULL
    );

    -- визиты на сайт до создания организации
    $df = (SELECT 
        DISTINCT
            COALESCE(v.yuid, c.yuid) as yuid,
            c.organization_id as organization_id,
            COALESCE(v.visit_id, c.visit_id) as visit_id,
            FIRST_VALUE(COALESCE(v.visit_id, c.visit_id)) OVER w as first_visit_id,
            FIRST_VALUE(COALESCE(v.event_datetime_msk, c.event_datetime_msk) ) OVER w as first_visit_datetime_msk,
            LAST_VALUE(COALESCE(v.visit_id, c.visit_id)) OVER w as last_visit_id,
            COUNT(COALESCE(v.visit_id, c.visit_id)) OVER w as cnt_visits,
            COALESCE(v.trafic_source_id, c.trafic_source_id) as trafic_source_id,
            COALESCE(v.traffic_source_name, c.traffic_source_name) as traffic_source_name,
            'site_visit' as event_type,
            COALESCE(v.event_datetime_msk, c.event_datetime_msk) as event_datetime_msk,
            COALESCE(v.event_date_msk, c.event_date_msk) as event_date_msk,
            COALESCE(v.channel, c.channel, '') as channel,
            COALESCE(v.channel_marketing_influenced, c.channel_marketing_influenced, '')  as channel_marketing_influenced,
            COALESCE(v.utm_campaign, c.utm_campaign, '')  as utm_campaign,
            COALESCE(v.utm_source, c.utm_source, '') as utm_source,
            COALESCE(v.utm_medium, c.utm_medium, '') as utm_medium,
            COALESCE(v.utm_content, c.utm_content, '') as utm_content,
            COALESCE(v.utm_term, c.utm_term, '') as utm_term,
            AsList('First','Last','Uniform') as attribution_type
        FROM $dl_visits as v
        FULL JOIN $dl_create as c
            ON 
                v.yuid = c.yuid
        WHERE 1=1
            AND (v.event_datetime_msk <= c.event_datetime_msk 
                    OR c.event_datetime_msk is NULL -- еще не создали даталенс
                    OR v.event_datetime_msk is NULL) 
        WINDOW w AS ( -- window specification for the named window "w" used above
                    PARTITION BY v.yuid, c.organization_id -- partitioning clause
                    ORDER BY v.event_datetime_msk ASC    -- sorting clause
                    ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING
                    )
    );

    $result_ma = (
        SELECT 
            yuid,
            organization_id,
            first_visit_datetime_msk,
            first_visit_id,
            last_visit_id,
            cnt_visits,
            trafic_source_id,
            traffic_source_name,
            visit_id,
            event_type,
            event_date_msk,
            event_datetime_msk,
            channel,
            channel_marketing_influenced,
            utm_campaign,
            $extract_campaign(utm_campaign, 0) as utm_campaign_traffic_soure,
            $extract_campaign(utm_campaign, 1) as utm_campaign_country,
            $extract_campaign(utm_campaign, 2) as utm_campaign_city,
            $extract_campaign(utm_campaign, 3) as utm_campaign_device,
            $extract_campaign(utm_campaign, 4) as utm_campaign_budget,
            $extract_campaign(utm_campaign, 5) as utm_campaign_product,
            $extract_campaign(utm_campaign, 6) as utm_campaign_business_unit,
            utm_source,
            utm_medium,
            utm_content,
            utm_term,
            attribution_type,
            IF(visit_id = first_visit_id, 1, 0) as event_first_weight,
            IF(visit_id = last_visit_id, 1, 0) as event_last_weight,
            CAST(1 as double)/CAST(cnt_visits as double) as event_uniform_weight
        FROM $df as d
        FLATTEN BY attribution_type
    );

INSERT INTO $result_ma_path WITH TRUNCATE
SELECT *
FROM $result_ma
ORDER BY yuid, organization_id
;

$funnel_step_2 = (
    SELECT 
        ma.yuid as yuid,
        c.organization_id as organization_id,
        ma.first_visit_datetime_msk as first_visit_datetime_msk,
        ma.first_visit_id as first_visit_id,
        ma.last_visit_id as last_visit_id,
        ma.cnt_visits as cnt_visits,
        ma.trafic_source_id as trafic_source_id,
        ma.traffic_source_name as traffic_source_name,
        c.visit_id as visit_id,
        c.event_type as event_type,
        c.event_date_msk as event_date_msk,
        c.event_datetime_msk as event_datetime_msk,
        ma.channel as channel,
        ma.channel_marketing_influenced as channel_marketing_influenced,
        ma.utm_campaign as utm_campaign,
        ma.utm_campaign_traffic_soure as utm_campaign_traffic_soure,
        ma.utm_campaign_country as utm_campaign_country,
        ma.utm_campaign_city as utm_campaign_city,
        ma.utm_campaign_device as utm_campaign_device,
        ma.utm_campaign_budget as utm_campaign_budget,
        ma.utm_campaign_product as utm_campaign_product,
        ma.utm_campaign_business_unit as utm_campaign_business_unit,
        ma.utm_source as utm_source,
        ma.utm_medium as utm_medium,
        ma.utm_content as utm_content,
        ma.utm_term as utm_term,
        ma.attribution_type as attribution_type,
        ma.event_first_weight as event_first_weight, 
        ma.event_last_weight as event_last_weight,
        ma.event_uniform_weight as event_uniform_weight
    FROM $result_ma as ma
    LEFT JOIN $dl_create as c
        ON ma.organization_id = c.organization_id 
    where c.yuid is not null
);

$funnel_step_3 = (
    SELECT 
        f.yuid as yuid,
        if(f.organization_id = r.organization_id, 0, 1) as check_org,
        r.organization_id as organization_id,
        f.first_visit_datetime_msk as first_visit_datetime_msk,
        f.first_visit_id as first_visit_id,
        f.last_visit_id as last_visit_id,
        f.cnt_visits as cnt_visits,
        f.trafic_source_id as trafic_source_id,
        f.traffic_source_name as traffic_source_name,
        r.request_id as request_id,
        r.event_type as event_type,
        r.event_date_msk as event_date_msk,
        r.event_datetime_msk as event_datetime_msk,
        f.channel as channel,
        f.channel_marketing_influenced as channel_marketing_influenced,
        f.utm_campaign as utm_campaign,
        f.utm_campaign_traffic_soure as utm_campaign_traffic_soure,
        f.utm_campaign_country as utm_campaign_country,
        f.utm_campaign_city as utm_campaign_city,
        f.utm_campaign_device as utm_campaign_device,
        f.utm_campaign_budget as utm_campaign_budget,
        f.utm_campaign_product as utm_campaign_product,
        f.utm_campaign_business_unit as utm_campaign_business_unit,
        f.utm_source as utm_source,
        f.utm_medium as utm_medium,
        f.utm_content as utm_content,
        f.utm_term as utm_term,
        f.attribution_type as attribution_type,
        f.event_first_weight as event_first_weight, 
        f.event_last_weight as event_last_weight,
        f.event_uniform_weight as event_uniform_weight
    FROM $funnel_step_2 as f
    LEFT JOIN $first_request as r
        ON f.organization_id = r.organization_id 
    where r.organization_id is not null -- только активированные даталенсы (есть запрос)
);

    $funnel = (
        SELECT 
            yuid,
            organization_id,
            first_visit_datetime_msk,
            first_visit_id,
            last_visit_id,
            cnt_visits,
            trafic_source_id,
            traffic_source_name,
            visit_id,
            NULL as request_id,
            CAST(visit_id as string) as event_id,
            event_type,
            event_datetime_msk,
            event_date_msk,
            channel,
            channel_marketing_influenced,
            utm_campaign,
            utm_campaign_traffic_soure,
            utm_campaign_country,
            utm_campaign_city,
            utm_campaign_device,
            utm_campaign_budget,
            utm_campaign_product,
            utm_campaign_business_unit,
            utm_source,
            utm_medium,
            utm_content,
            utm_term,
            attribution_type,
            event_first_weight,
            event_last_weight,
            event_uniform_weight
        FROM $result_ma

        UNION ALL

        SELECT 
            yuid,
            organization_id,
            first_visit_datetime_msk,
            first_visit_id,
            last_visit_id,
            cnt_visits,
            trafic_source_id,
            traffic_source_name,
            visit_id,
            NULL as request_id,
            CAST(visit_id as string) as event_id,
            event_type,
            event_datetime_msk,
            event_date_msk,
            channel,
            channel_marketing_influenced,
            utm_campaign,
            utm_campaign_traffic_soure,
            utm_campaign_country,
            utm_campaign_city,
            utm_campaign_device,
            utm_campaign_budget,
            utm_campaign_product,
            utm_campaign_business_unit,
            utm_source,
            utm_medium,
            utm_content,
            utm_term,
            attribution_type,
            event_first_weight,
            event_last_weight,
            event_uniform_weight
        FROM $funnel_step_2

        UNION ALL

        SELECT 
            yuid,
            organization_id,
            first_visit_datetime_msk,
            first_visit_id,
            last_visit_id,
            cnt_visits,
            trafic_source_id,
            traffic_source_name,
            NULL as visit_id,
            request_id,
            request_id as event_id,
            event_type,
            event_datetime_msk,
            event_date_msk,
            channel,
            channel_marketing_influenced,
            utm_campaign,
            utm_campaign_traffic_soure,
            utm_campaign_country,
            utm_campaign_city,
            utm_campaign_device,
            utm_campaign_budget,
            utm_campaign_product,
            utm_campaign_business_unit,
            utm_source,
            utm_medium,
            utm_content,
            utm_term,
            attribution_type,
            event_first_weight,
            event_last_weight,
            event_uniform_weight
        FROM $funnel_step_3
    );

    INSERT INTO $result_funnel_path WITH TRUNCATE
    SELECT *
    FROM $funnel
    ORDER BY yuid, organization_id
    ;

END DEFINE;

EXPORT $datalens_marketing_attribution_script;
