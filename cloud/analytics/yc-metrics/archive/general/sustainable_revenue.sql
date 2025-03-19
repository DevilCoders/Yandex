/*___DOC___
====Sustainable Revenue
Устойчивая выручка - выручка за неделю без ряда sku, потребление по которым начисляется в конце месяца
<{График
{{iframe frameborder="0" width="70%" height="300px" src="https://charts.yandex-team.ru/preview/editor/hv5twavvj2jqb?metric_name=Sustainable%20Revenue&6112cac5-7432-4748-a900-99da3f669075=Sustainable%20Revenue&metricName=Sustainable%20Revenue&_embedded=1"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/sustainable_revenue.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;

IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $sustainable_revenue() AS 
    DEFINE SUBQUERY $sustainable_revenue_ungrouped() AS
        SELECT 
            $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as `date`,
            real_consumption_vat
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            is_sustainable = 1
            and event = 'day_use'
            and $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
    END DEFINE;
    DEFINE SUBQUERY $sustainable_revenue_grouped($param) AS 
        SELECT 
            `date`,
            'sustainable_revenue' as metric_id,
            'Sustainable Revenue' as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            $period as period,
            SUM(real_consumption_vat) as metric_value
        FROM $sustainable_revenue_ungrouped()
        GROUP BY `date`
    END DEFINE;

    SELECT 
    `date`,
    metric_id,
    metric_name,
    metric_group,
    metric_subgroup,
    metric_unit,
    metric_value,
    metric_value_prev,
    metric_growth_abs,
    metric_growth_pct,
/*    IF(
        metric_growth_pct-AVG(metric_growth_pct) OVER w>= 0.003, 
        'green',
        IF (
            metric_growth_pct-AVG(metric_growth_pct) OVER w >= -0.003,
            'yellow',
            'red'
        )
    ) as */
    metric_color
    FROM $metric_growth($sustainable_revenue_grouped, '', 'straight','standart', '')
--    WINDOW w AS (ORDER BY `date` ROWS BETWEEN 4 PRECEDING AND 1 PRECEDING ) 
    ORDER BY `date` DESC

END DEFINE;

EXPORT $sustainable_revenue;
