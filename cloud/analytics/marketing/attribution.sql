use hahn;

IMPORT apply_attribution SYMBOLS $apply_attribution, $apply_attribution_all;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

DEFINE SUBQUERY $res_array() AS (
SELECT 
    billing_account_id,
    ListSortAsc(
            AGGREGATE_LIST(
                (
                    COALESCE(CAST(event_id AS Uint64),0), 
                    event_type, 
                    COALESCE(CAST(event_time AS Uint64),0),
                    channel,
                    channel_marketing_influenced,
                    utm_campaign,
                    utm_campaign_name,
                    utm_campaign_name_1,
                    utm_campaign_name_2,
                    utm_campaign_name_3,
                    utm_campaign_name_4,
                    utm_campaign_name_5,
                    utm_campaign_name_6,
                    utm_campaign_name_7,
                    -- utm_campaign_id,
                    utm_source,
                    utm_medium,
                    utm_term,
                    -- counter_id_serial,
                )
            ), 
            ($x) -> {RETURN $x.2;}
        ) as ba_events,
    $apply_attribution_all(
        ListSortAsc(
            AGGREGATE_LIST(
                (
                    COALESCE(CAST(event_id AS Uint64),0), 
                    event_type, 
                    COALESCE(CAST(event_time AS Uint64),0),
                    channel,
                    channel_marketing_influenced,
                    utm_campaign,
                    utm_campaign_name,
                    utm_campaign_name_1,
                    utm_campaign_name_2,
                    utm_campaign_name_3,
                    utm_campaign_name_4,
                    utm_campaign_name_5,
                    utm_campaign_name_6,
                    utm_campaign_name_7,
                    -- utm_campaign_id,
                    utm_source,
                    utm_medium,
                    utm_term,
                    -- counter_id_serial
                )
            ), 
            ($x) -> {RETURN $x.2;}
        )
        , 
        'ba_created'
    ) as ba_marketing_attribution_weights
FROM (
    SELECT 
        billing_account_id,
        event_time,
        event_type,
        event_id,
        coalesce(channel, '') as channel,
        coalesce(channel_marketing_influenced, '') as channel_marketing_influenced,
        coalesce(utm_campaign, '') as utm_campaign,
        coalesce(utm_campaign_name, '') as  utm_campaign_name,
        coalesce(utm_campaign_name_1, '') as  utm_campaign_name_1,
        coalesce(utm_campaign_name_2, '') as  utm_campaign_name_2,
        coalesce(utm_campaign_name_3, '') as  utm_campaign_name_3,
        coalesce(utm_campaign_name_4, '') as  utm_campaign_name_4,
        coalesce(utm_campaign_name_5, '') as  utm_campaign_name_5,
        coalesce(utm_campaign_name_6, '') as  utm_campaign_name_6,
        coalesce(utm_campaign_name_7, '') as  utm_campaign_name_7,
        -- coalesce(utm_campaign_id, '') as  utm_campaign_id,
        coalesce(utm_source, '') as utm_source,
        coalesce(utm_medium, '') as utm_medium,
        coalesce(utm_term, '') as utm_term,
        -- coalesce(counter_id_serial, 1) as counter_id_serial
    FROM (
        SELECT
            String::SplitToList(billing_account_id, ',') as billing_account_id,
            event_time,
            event_type,
            event_id,
            channel,
            channel_marketing_influenced,
            utm_campaign,
            utm_campaign_name,
            utm_campaign_name_1,
            utm_campaign_name_2,
            utm_campaign_name_3,
            utm_campaign_name_4,
            utm_campaign_name_5,
            utm_campaign_name_6,
            utm_campaign_name_7,
            -- utm_campaign_id,
            utm_source,
            utm_medium,
            utm_term,
            -- counter_id_serial
        FROM `//home/cloud_analytics/marketing/events_for_attribution`
        WHERE 1=1
        AND ((counter_id in (50027884, 51465824)) OR event_type != 'visit')
    )
    FLATTEN BY billing_account_id
    WHERE billing_account_id is not null and billing_account_id != ''
    -- AND billing_account_id = 'dn2003nke4qtrmuhn4vm'
)
GROUP BY billing_account_id
)
END DEFINE;

DEFINE SUBQUERY  $res_flat() AS (
    SELECT 
        a.billing_account_id,
        a.ba_marketing_attribution_weights,
        a.ba_marketing_attribution_weights.0 as marketing_attribution_event_id,
        a.ba_marketing_attribution_weights.1 as marketing_attribution_event_type,
        a.ba_marketing_attribution_weights.2 as marketing_attribution_event_time,
        a.ba_marketing_attribution_weights.3 as marketing_attribution_channel,
        a.ba_marketing_attribution_weights.4 as marketing_attribution_channel_marketing_influenced,
        a.ba_marketing_attribution_weights.5 as marketing_attribution_utm_campaign,
        a.ba_marketing_attribution_weights.6 as marketing_attribution_utm_campaign_name,
        a.ba_marketing_attribution_weights.7 as marketing_attribution_utm_campaign_name_1,
        a.ba_marketing_attribution_weights.8 as marketing_attribution_utm_campaign_name_2,
        a.ba_marketing_attribution_weights.9 as marketing_attribution_utm_campaign_name_3,
        a.ba_marketing_attribution_weights.10 as marketing_attribution_utm_campaign_name_4,
        a.ba_marketing_attribution_weights.11 as marketing_attribution_utm_campaign_name_5,
        a.ba_marketing_attribution_weights.12 as marketing_attribution_utm_campaign_name_6,
        a.ba_marketing_attribution_weights.13 as marketing_attribution_utm_campaign_name_7,
        -- a.ba_marketing_attribution_weights.14 as marketing_attribution_utm_campaign_id,
        '' as marketing_attribution_utm_campaign_id,
        a.ba_marketing_attribution_weights.14 as marketing_attribution_utm_source,
        a.ba_marketing_attribution_weights.15 as marketing_attribution_utm_medium,
        a.ba_marketing_attribution_weights.16 as marketing_attribution_utm_term,
        -- a.ba_marketing_attribution_weights.17 as marketing_attribution_counter_id_serial,
        a.ba_marketing_attribution_weights.17[0] as marketing_attribution_event_first_weight,
        a.ba_marketing_attribution_weights.17[1] as marketing_attribution_event_last_weight,
        a.ba_marketing_attribution_weights.17[2] as marketing_attribution_event_uniform_weight,
        a.ba_marketing_attribution_weights.17[3] as marketing_attribution_event_u_shape_weight,
        a.ba_marketing_attribution_weights.17[4] as marketing_attribution_event_exp_7d_half_life_time_decay_weight,
    FROM $res_array() as a
    FLATTEN BY ba_marketing_attribution_weights
);
END DEFINE;

DEFINE ACTION $attribution_array($path) AS 
    INSERT INTO $path WITH TRUNCATE 
    SELECT *
    FROM $res_array();
END DEFINE;

DEFINE ACTION $attribution_flat($path) AS 
    INSERT INTO $path WITH TRUNCATE 
    SELECT *
    FROM $res_flat();
END DEFINE;

EXPORT $attribution_array, $attribution_flat;
