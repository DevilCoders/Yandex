
/*___DOC___
====Marketing Performance Costs
Сумма потраченная на Performance за неделю по ((https://yt.yandex-team.ru/hahn/navigation?path=//home/marketing-data/money_log/v2/datasets/project_2 данным)) операционного маркетинга. 
<{График
{{iframe frameborder="0" width="70%" height="300px" src="https://charts.yandex-team.ru/preview/editor/hv5twavvj2jqb?metric_name=Marketing%20Performance%20Costs&6112cac5-7432-4748-a900-99da3f669075=Marketing%20Performance%20Costs&metricName=Marketing%20Performance%20Costs&_embedded=1"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/marketing_performance_costs.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $marketing_performance_costs() AS
    DEFINE SUBQUERY $res($params) AS
        SELECT
            `date`,
            'mrkt_performance_costs' as metric_id,
            'Marketing Performance Costs' AS metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            $params as period,
            sum(cost) as metric_value
        FROM (
            SELECT  
                --$format_date(DateTime::StartOfWeek($parse_date(`date`))) as `date`,
                $round_period_date($params,`date`) as `date`,
                cost
            --FROM `//home/marketing-data/money_log/v2/datasets/project_2`
            FROM `//home/marketing-data/money_log/datasets/project_2/project_2`
        )
        WHERE
            --`date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            `date` < $round_current_time($params)
        GROUP BY 
            `date`
    END DEFINE;

   DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()
END DEFINE;
EXPORT $marketing_performance_costs;