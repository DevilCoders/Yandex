/*___DOC___
==== Revenue Plan
План по выручке по неделям

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/revenue_plan_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT tables SYMBOLS $last_non_empty_table;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $revenue_plan_all() AS
    DEFINE SUBQUERY $res($segment,$period) AS
        SELECT 
            `date`,
            'revenue_plan' as metric_id,
            'Revenue Plan' as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(plan) as metric_value
        FROM (
            SELECT 
                --$format_date(DateTime::StartOfWeek($parse_date(`date`))) as `date`,
                $round_period_date($period,`date`) as `date`,
                plan 
            FROM 
                `//home/cloud_analytics/marketing/plan_daily`
        )
        GROUP BY 
            `date`
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

EXPORT $revenue_plan_all;