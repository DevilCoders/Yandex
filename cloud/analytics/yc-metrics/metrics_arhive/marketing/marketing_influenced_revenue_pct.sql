/*___DOC___
==== Marketing Influenced Revenue pct
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/marketing_influenced_revenue_pct.sql" formatter="yql"}}
}>
___DOC___*/


use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $marketing_influenced_revenue_pct() AS
    DEFINE SUBQUERY $res($segment,$period) AS 
        SELECT 
            `date`,
            'mrkt_influenced_rev_pct_' || $segment[0][0]  as metric_id,
            'Marketing Influenced Revenue Pct' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit,
            0 as is_ytd,
            'week' as period,
            SUM_IF(real_consumption_vat * marketing_attribution_event_exp_7d_half_life_time_decay_weight, marketing_attribution_channel_marketing_influenced = 'Marketing') / SUM(real_consumption_vat * marketing_attribution_event_exp_7d_half_life_time_decay_weight) *100 as metric_value
        FROM (
        SELECT 
            $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as `date`,
            real_consumption_vat,
            marketing_attribution_event_exp_7d_half_life_time_decay_weight,
            marketing_attribution_channel_marketing_influenced
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube_with_marketing_attribution`
        WHERE 1=1
            and event = 'day_use'
            and $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            AND crm_account_segment in $segment[1]
        )
        GROUP BY `date`
    END DEFINE;

    SELECT *
    FROM $metric_growth($res, AsList(AsList('Enterprise'),AsList('Enterprise')),'', 'straight', 'standart', '')
    UNION ALL 
    SELECT *
    FROM $metric_growth($res, AsList(AsList('SMB'), AsList('Mass','Medium')),'', 'straight', 'standart', '');

END DEFINE;

EXPORT $marketing_influenced_revenue_pct;