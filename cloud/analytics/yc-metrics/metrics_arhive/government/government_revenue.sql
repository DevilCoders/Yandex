/*___DOC___
====Enterprise Revenue
Выручка сегмента Enterprise по неделям
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/enterprise_revenue.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $government_revenue() AS
    DEFINE SUBQUERY $res($param,$period) AS 
        SELECT 
            `date`,
            'government_revenue' as metric_id,
            'Government Revenue' as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(real_consumption_vat) as metric_value
        FROM (
            SELECT 
                $round_period_datetime($period,event_time) as `date`,
                real_consumption_vat
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE 
                event = 'day_use'
                and crm_account_segment = 'Public sector'
                and if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
                    $round_period_datetime($period,event_time) < $round_current_time($period))
        )
        GROUP BY `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, '', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $government_revenue;