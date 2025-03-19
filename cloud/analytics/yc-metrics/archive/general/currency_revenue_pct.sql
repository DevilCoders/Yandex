/*___DOC___
====Currency Revenue %
Доля валютной выручки по всем сегментам (выручки, полученной в USD и EUR) относительно общей выручки по неделям.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/currency_revenue_pct.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $currency_revenue_pct() AS
    DEFINE SUBQUERY $res($param,$period) AS 
        SELECT 
            `date`,
            'currency_revenue_pct' as metric_id,
            'Currency Revenue pct' as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $period as period,
            Math::Round(sum(if(ba_currency in ('USD','EUR'),real_consumption_vat,0))/sum(real_consumption_vat) ,-4)*100  as metric_value
        FROM (
            SELECT 
                $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as `date`,
                real_consumption_vat,
                ba_currency
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE 
                event = 'day_use'
                and $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        )
        GROUP BY `date`
    END DEFINE;

    SELECT *
    FROM $metric_growth($res, '', 'straight', 'standart', '');

END DEFINE;

EXPORT $currency_revenue_pct;